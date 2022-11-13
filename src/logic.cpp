#include <algorithm>
#include <cmath>
#include <iostream>

#include "app.hpp"
#include "logic.hpp"

namespace {

constexpr float kPaddleWidth = 0.2f;
constexpr float kPaddleHeight = 0.05f;
constexpr float kBallRadius = 0.025f;
constexpr float kBallDiameter = kBallRadius * 2.0f;
constexpr float kBallSpeed = 0.5f;

constexpr int kComputerBallIndex = 0;
constexpr int kPlayerBallIndex = 1;

float computer_position = 0.0f;
float player_position = 0.0f;

struct vec2 {
  float x, y;

  vec2 (float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

  vec2 operator* (float s) const { return vec2(x * s, y * s); }

  vec2& operator+= (const vec2& o) { x += o.x; y += o.y; return *this; }
};

enum class ball_attachment_state { none, computer, player };

struct ball {
  vec2 position;
  vec2 velocity;
  ball_attachment_state attachment_state;

  ball (ball_attachment_state attachment_state) : attachment_state(attachment_state) {}

  void maybe_release () {
    switch (attachment_state) {
      case ball_attachment_state::none:
        return;

      case ball_attachment_state::computer:
        velocity = vec2(-M_SQRT1_2, -M_SQRT1_2) * kBallSpeed;
        break;

      case ball_attachment_state::player:
        velocity = vec2(M_SQRT1_2, M_SQRT1_2) * kBallSpeed;
        break;
    }
    attachment_state = ball_attachment_state::none;
  }

  void tick (float dt) {
    constexpr float kBallAttachmentOffset = kPaddleWidth * 0.125f;
    constexpr float kBallAttachmentY = 0.5f / kAspect - kPaddleHeight - kBallRadius;
    switch (attachment_state) {
      case ball_attachment_state::none:
        position += velocity * dt;
        break;

      case ball_attachment_state::computer:
        position = vec2(computer_position - kBallAttachmentOffset, kBallAttachmentY);
        break;

      case ball_attachment_state::player:
        position = vec2(player_position + kBallAttachmentOffset, -kBallAttachmentY);
        break;
    }
    ball_program->draw_quad(position.x, position.y, kBallDiameter, kBallDiameter);
  }
  
} balls[] {ball_attachment_state::computer, ball_attachment_state::player};

}

void set_player_position (float position) {
  constexpr float kMaxPos = 0.5f - kPaddleWidth * 0.5f;
  player_position = std::min(std::max(position, -kMaxPos), kMaxPos);
}

float get_player_position () {
  return player_position;
}

void maybe_release_player_ball () {
  balls[kPlayerBallIndex].maybe_release();
}

void tick (float dt) {
  backdrop_program->draw_quad(0.0f, 0.0f, 1.0f, 1.0f / kAspect);

  constexpr float kPaddleY = 0.5f / kAspect - kPaddleHeight * 0.5f;
  paddle_program->draw_quad(computer_position, kPaddleY, kPaddleWidth, kPaddleHeight);
  paddle_program->draw_quad(player_position, -kPaddleY, kPaddleWidth, kPaddleHeight);

  for (ball& ball : balls) ball.tick(dt);
}