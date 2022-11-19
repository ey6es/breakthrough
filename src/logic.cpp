#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>

#include "app.hpp"
#include "logic.hpp"

namespace {

constexpr float kPaddleWidth = 0.2f;
constexpr float kPaddleHeight = 0.05f;
constexpr float kPaddleY = 0.5f / kAspect - kPaddleHeight * 0.5f;
constexpr float kMaxPaddleX = 0.5f - kPaddleWidth * 0.5f;

constexpr float kBallRadius = 0.025f;
constexpr float kBallDiameter = kBallRadius * 2.0f;
constexpr float kBallSpeed = 0.5f;

constexpr float kFieldHeight = 1.0f;

constexpr int kComputerBallIndex = 0;
constexpr int kPlayerBallIndex = 1;

float computer_position = 0.0f;
float player_position = 0.0f;

std::bitset<kFieldRows * kFieldCols> block_states;

struct vec2 {
  float x, y;

  vec2 (float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  vec2 operator+ (const vec2& o) const { return vec2(x + o.x, y + o.y); }
  vec2 operator- (const vec2& o) const { return vec2(x - o.x, y - o.y); }
  vec2 operator- () const { return vec2(-x, -y); }
  vec2 operator* (float s) const { return vec2(x * s, y * s); }
  vec2 operator/ (float s) const { return *this * (1.0f / s); }
  
  vec2& operator+= (const vec2& o) { x += o.x; y += o.y; return *this; }
  vec2& operator-= (const vec2& o) { x -= o.x; y -= o.y; return *this; }
  vec2& operator*= (float s) { x *= s; y *= s; return *this; }
  vec2& operator/= (float s) { return *this *= (1.0f / s); }

  float length_squared () const { return x * x + y * y; }
  float length () const { return std::hypot(x, y); }
  vec2 normalize () const { return *this / length(); }
  vec2 ortho () const { return vec2(y, -x); }
  float dot (const vec2& o) const { return x * o.x + y * o.y; }
  vec2 reflect (const vec2& n) const { return n * dot(n) * 2.0f - *this; }
};

template<typename T>
T clamp (T value, T min, T max) {
  return std::min(std::max(value, min), max);
}

class Ball {
public:
  Ball (bool player_owned) : player_owned_(player_owned) {}

  const vec2& get_position () const { return position_; }
  const vec2& get_velocity () const { return velocity_; }

  bool is_attached () const { return attached_; }

  void maybe_release () {
    if (!attached_) return;
    velocity_ = vec2(M_SQRT1_2, M_SQRT1_2) * kBallSpeed * (player_owned_ ? 1.0f : -1.0f);
    attached_ = false;
    play_launch(player_owned_);
  }

  void tick (float dt) {
    if (attached_) {
      constexpr float kBallAttachmentOffset = kPaddleWidth * 0.125f;
      constexpr float kBallAttachmentY = 0.5f / kAspect - kPaddleHeight - kBallRadius;
      if (player_owned_) position_ = vec2(player_position + kBallAttachmentOffset, -kBallAttachmentY);
      else position_ = vec2(computer_position - kBallAttachmentOffset, kBallAttachmentY);

    } else {
      position_ += velocity_ * dt;
      check_collisions();
    }
    ball_program->draw_quad(position_.x, position_.y, kBallDiameter, kBallDiameter);
  }

private:
  bool player_owned_;
  vec2 position_;
  vec2 velocity_;
  bool attached_ = true;

  void check_collisions () {
    constexpr float kMaxY = 0.5f / kAspect + kBallRadius;
    if (position_.y < -kMaxY || position_.y > kMaxY) {
      attached_ = true;
      play_loss(player_owned_);
      return;
    }
    // check against blocks
    auto min_col = clamp((int)((position_.x - kBallRadius + 0.5f) * kFieldCols), 0, kFieldCols - 1);
    auto max_col = clamp((int)((position_.x + kBallRadius + 0.5f) * kFieldCols), 0, kFieldCols - 1);
    constexpr float kHalfHeight = kFieldHeight * 0.5f;
    constexpr float kFieldRowScale = kFieldRows / kFieldHeight;
    auto min_row = clamp((int)((position_.y - kBallRadius + kHalfHeight) * kFieldRowScale), 0, kFieldRows - 1);
    auto max_row = clamp((int)((position_.y + kBallRadius + kHalfHeight) * kFieldRowScale), 0, kFieldRows - 1);
    for (auto row = min_row; row <= max_row; ++row) {
      for (auto col = min_col; col <= max_col; ++col) {
        auto index = row * kFieldCols + col;
        constexpr float kBlockWidth = 1.0f / kFieldCols;
        constexpr float kBlockHeight = kFieldHeight / kFieldRows;
        if (!block_states.test(index)) {
          if (check_quad_collision(
              (col + 0.5f) * kBlockWidth - 0.5f,
              (row + 0.5f) * kBlockHeight - kHalfHeight,
              kBlockWidth, kBlockHeight)) {
            block_states.set(index);
            clear_block(row, col);
          }
        }
      }
    }

    // check against left and right walls
    check_segment_collision(vec2(-0.5f, -kMaxY), vec2(-0.5f, kMaxY));
    check_segment_collision(vec2(0.5f, kMaxY), vec2(0.5f, -kMaxY));

    // check against paddles
    check_paddle_collision(computer_position, kPaddleY);
    check_paddle_collision(player_position, -kPaddleY);
  }

  bool check_quad_collision (float x, float y, float w, float h) {
    float hw = w * 0.5f, hh = h * 0.5f;
    return
      check_segment_collision(vec2(x - hw, y - hh), vec2(x + hw, y - hh)) |
      check_segment_collision(vec2(x + hw, y - hh), vec2(x + hw, y + hh)) |
      check_segment_collision(vec2(x + hw, y + hh), vec2(x - hw, y + hh)) |
      check_segment_collision(vec2(x - hw, y + hh), vec2(x - hw, y - hh));
  }

  bool check_segment_collision (const vec2& a, const vec2& b) {
    auto ap = position_ - a;
    auto ab = b - a;
    auto normal = ab.ortho();
    auto normal_dot = normal.dot(ap);
    if (normal_dot < 0.0f) return false; // wrong side

    auto dir_dot = ap.dot(ab);
    auto length_squared = ab.length_squared();
    vec2 closest;
    if (dir_dot < 0.0f) closest = a;
    else if (dir_dot > length_squared) closest = b;
    else closest = a + ab * (dir_dot / length_squared); 

    normal = position_ - closest;
    auto length = normal.length();
    auto penetration = kBallRadius - length;
    if (penetration <= 0.0f) return false;
    normal /= length;
    position_ += normal * penetration;
    velocity_ = (-velocity_).reflect(normal);

    play_bounce(player_owned_);
    return true;
  }

  void check_paddle_collision (float x, float y) {
    vec2 a(x - kPaddleWidth * 0.5f, y);
    vec2 b(x + kPaddleWidth * 0.5f, y);
    auto ap = position_ - a;
    auto ab = b - a;
    
    auto dir_dot = ap.dot(ab);
    auto length_squared = ab.length_squared();
    vec2 closest;
    if (dir_dot < 0.0f) closest = a;
    else if (dir_dot > length_squared) closest = b;
    else closest = a + ab * (dir_dot / length_squared);

    auto normal = position_ - closest;
    auto length = normal.length();
    auto penetration = kBallRadius + kPaddleHeight * 0.5f - length;
    if (penetration <= 0.0f) return;
    normal /= length;
    position_ += normal * penetration;

    constexpr float kCurveFactor = 0.5f / kPaddleWidth;
    normal.x += (closest.x - x) * kCurveFactor;
    velocity_ = (-velocity_).reflect(normal.normalize());

    play_bounce(player_owned_);
  }
} balls[] {false, true};

void tick_computer (float dt) {
  static float target_position = 0.0f;
  auto& own_ball = balls[kComputerBallIndex];
  if (own_ball.is_attached()) {
    static bool target_position_initialized = false;
    if (!target_position_initialized) {
      static std::default_random_engine engine(std::chrono::system_clock::now().time_since_epoch().count());
      target_position = std::uniform_real_distribution<float>(-kMaxPaddleX, kMaxPaddleX)(engine);
      target_position_initialized = true;
    }
    if (computer_position == target_position) {
      own_ball.maybe_release();
      target_position_initialized = false;
    }
  } else {
    auto it = std::max_element(std::begin(balls), std::end(balls), [](auto& a, auto& b) {
      return
        (a.get_position().y > 0.0f) < (b.get_position().y > 0.0f) || // on our side
        (a.get_velocity().y > 0.0f) < (b.get_velocity().y > 0.0f) || // approaching us
        a.get_position().y < b.get_position().y; // closer
    });
    target_position = it->get_position().x;
  }
  constexpr float kComputerSpeed = 0.35f;
  auto velocity = (computer_position < target_position) ? kComputerSpeed : -kComputerSpeed;
  computer_position = (computer_position < target_position)
    ? std::min(computer_position + kComputerSpeed * dt, target_position)
    : std::max(computer_position - kComputerSpeed * dt, target_position);
  computer_position = clamp(computer_position, -kMaxPaddleX, kMaxPaddleX);
}

}

void set_player_position (float position) {
  player_position = clamp(position, -kMaxPaddleX, kMaxPaddleX);
}

float get_player_position () {
  return player_position;
}

bool get_block_state (int row, int col) {
  return block_states.test(row * kFieldCols + col);
}

void maybe_release_player_ball () {
  balls[kPlayerBallIndex].maybe_release();
}

void tick (float dt) {
  backdrop_program->draw_quad(0.0f, 0.0f, 1.0f, 1.0f / kAspect);

  tick_computer(dt);

  paddle_program->draw_quad(computer_position, kPaddleY, kPaddleWidth, kPaddleHeight);
  paddle_program->draw_quad(player_position, -kPaddleY, kPaddleWidth, kPaddleHeight);

  blocks_program->draw_quad(0.0f, 0.0f, 1.0f, kFieldHeight);

  for (auto& ball : balls) ball.tick(dt);
}