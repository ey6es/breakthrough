#include <algorithm>
#include <bitset>
#include <cmath>
#include <iostream>

#include "app.hpp"
#include "logic.hpp"

namespace {

constexpr float kPaddleWidth = 0.2f;
constexpr float kPaddleHeight = 0.05f;
constexpr float kPaddleY = 0.5f / kAspect - kPaddleHeight * 0.5f;

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

  void maybe_release () {
    if (!attached_) return;
    velocity_ = vec2(M_SQRT1_2, M_SQRT1_2) * kBallSpeed * (player_owned_ ? 1.0f : -1.0f);
    attached_ = false;
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
    check_quad_collision(computer_position, kPaddleY, kPaddleWidth, kPaddleHeight);
    check_quad_collision(player_position, -kPaddleY, kPaddleWidth, kPaddleHeight);
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
    if (dir_dot < 0.0f) return check_point_collision(a);
    else if (dir_dot > length_squared) return check_point_collision(b);

    auto length = std::sqrt(length_squared);
    auto penetration = kBallRadius - normal_dot / length;
    if (penetration <= 0.0f) return false;
    normal /= length;
    position_ += normal * penetration;
    velocity_ = (-velocity_).reflect(normal);
    return true;
  }

  bool check_point_collision (const vec2& a) {
    auto normal = position_ - a;
    auto length = normal.length();
    auto penetration = kBallRadius - length;
    if (penetration <= 0.0f) return false;
    normal /= length;
    position_ += normal * penetration;
    velocity_ = (-velocity_).reflect(normal);
    return true;
  }
} balls[] {false, true};

}

void set_player_position (float position) {
  constexpr float kMaxPos = 0.5f - kPaddleWidth * 0.5f;
  player_position = clamp(position, -kMaxPos, kMaxPos);
}

float get_player_position () {
  return player_position;
}

void maybe_release_player_ball () {
  balls[kPlayerBallIndex].maybe_release();
}

void tick (float dt) {
  backdrop_program->draw_quad(0.0f, 0.0f, 1.0f, 1.0f / kAspect);

  paddle_program->draw_quad(computer_position, kPaddleY, kPaddleWidth, kPaddleHeight);
  paddle_program->draw_quad(player_position, -kPaddleY, kPaddleWidth, kPaddleHeight);

  blocks_program->draw_quad(0.0f, 0.0f, 1.0f, kFieldHeight);

  for (auto& ball : balls) ball.tick(dt);
}