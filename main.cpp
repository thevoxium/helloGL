#define GL_SILENCE_DEPRECATION
#include "math.hpp"
#include <FTGL/ftgl.h>
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

double MOTION_TIME = 0.016;
double GRAVITY_SCALE = 0.01;
double RESTITUTION = 0.7;
const int PHYSICS_STEPS = 4;
const double PUSH_FACTOR = 1.05;
const double MIN_VELOCITY = 0.001;
const double MAX_VELOCITY = 2.0;

struct Circle {
  double x, y, radius;
  vec2 position, velocity, acc;
  double prevX, prevY;

  Circle() : x(0), y(0), radius(0), prevX(0), prevY(0) {
    position = {0.0, 0.0};
    velocity = {0.0, 0.0};
    acc = {0.0, -9.8 * GRAVITY_SCALE};
  }

  Circle(double x_, double y_, double radius_)
      : x(x_), y(y_), radius(radius_), prevX(x_), prevY(y_) {
    position = {x_, y_};
    velocity = {0.0, 0.0};
    acc = {0.0, -9.8 * GRAVITY_SCALE};
  }
};

FTGLPixmapFont *font = nullptr;
string mouseCoords = "X: 0, Y: 0";
vector<Circle> circles;

void errorCallback(int error, const char *message) {
  cerr << "GLFW Error " << error << ": " << message << endl;
}

void capVelocity(vec2 &velocity) {
  double speedSquared = velocity.x * velocity.x + velocity.y * velocity.y;
  if (speedSquared > MAX_VELOCITY * MAX_VELOCITY) {
    double speed = sqrt(speedSquared);
    velocity.x = (velocity.x / speed) * MAX_VELOCITY;
    velocity.y = (velocity.y / speed) * MAX_VELOCITY;
  }
}

bool checkLineCircleIntersection(double x1, double y1, double x2, double y2,
                                 double cx, double cy, double radius) {
  double dx = x2 - x1;
  double dy = y2 - y1;
  double a = dx * dx + dy * dy;
  double b = 2 * (dx * (x1 - cx) + dy * (y1 - cy));
  double c = cx * cx + cy * cy + x1 * x1 + y1 * y1 - 2 * (cx * x1 + cy * y1) -
             radius * radius;

  double discriminant = b * b - 4 * a * c;
  if (discriminant < 0)
    return false;

  discriminant = sqrt(discriminant);
  double t1 = (-b - discriminant) / (2 * a);
  double t2 = (-b + discriminant) / (2 * a);

  return (t1 >= 0 && t1 <= 1) || (t2 >= 0 && t2 <= 1);
}

void resolveCollision(Circle &a, Circle &b) {
  vec2 normal = {b.x - a.x, b.y - a.y};
  double dist = sqrt(normal.x * normal.x + normal.y * normal.y);

  if (dist == 0) {
    a.x -= 0.001;
    b.x += 0.001;
    return;
  }

  normal = {normal.x / dist, normal.y / dist};

  double relativeVelocityX = b.velocity.x - a.velocity.x;
  double relativeVelocityY = b.velocity.y - a.velocity.y;
  double velocityAlongNormal =
      relativeVelocityX * normal.x + relativeVelocityY * normal.y;

  if (velocityAlongNormal > 0)
    return;

  double impulse = -(1 + RESTITUTION) * velocityAlongNormal * 0.5;

  vec2 impulseVec = {normal.x * impulse, normal.y * impulse};

  a.velocity.x -= impulseVec.x;
  a.velocity.y -= impulseVec.y;
  b.velocity.x += impulseVec.x;
  b.velocity.y += impulseVec.y;

  capVelocity(a.velocity);
  capVelocity(b.velocity);

  double overlap = (a.radius + b.radius) - dist;
  if (overlap > 0) {
    vec2 correction = {normal.x * overlap * PUSH_FACTOR * 0.5,
                       normal.y * overlap * PUSH_FACTOR * 0.5};

    a.x -= correction.x;
    a.y -= correction.y;
    a.position.x = a.x;
    a.position.y = a.y;

    b.x += correction.x;
    b.y += correction.y;
    b.position.x = b.x;
    b.position.y = b.y;
  }

  if (fabs(a.velocity.x) < MIN_VELOCITY)
    a.velocity.x = 0;
  if (fabs(a.velocity.y) < MIN_VELOCITY)
    a.velocity.y = 0;
  if (fabs(b.velocity.x) < MIN_VELOCITY)
    b.velocity.x = 0;
  if (fabs(b.velocity.y) < MIN_VELOCITY)
    b.velocity.y = 0;
}

void checkCollisions() {
  for (size_t i = 0; i < circles.size(); i++) {
    for (size_t j = i + 1; j < circles.size(); j++) {
      Circle &a = circles[i];
      Circle &b = circles[j];

      if (checkLineCircleIntersection(a.prevX, a.prevY, a.x, a.y, b.x, b.y,
                                      a.radius + b.radius) ||
          checkLineCircleIntersection(b.prevX, b.prevY, b.x, b.y, a.x, a.y,
                                      a.radius + b.radius)) {
        resolveCollision(a, b);
      }

      double dx = b.x - a.x;
      double dy = b.y - a.y;
      double distance = sqrt(dx * dx + dy * dy);

      if (distance < (a.radius + b.radius)) {
        resolveCollision(a, b);
      }
    }
  }
}

void drawCircle(Circle circle, int segments) {
  glLineWidth(2.0f);
  glBegin(GL_LINE_LOOP);

  int width, height;
  glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
  float aspectRatio = (float)width / height;

  const float angleStep = 2.0f * M_PI / segments;

  for (int i = 0; i < segments; i++) {
    float angle = i * angleStep;
    float x = circle.x + (circle.radius * cos(angle)) * (height / (float)width);
    float y = circle.y + circle.radius * sin(angle);
    glVertex2f(x, y);
  }

  glEnd();
}

void convertToNDC(GLFWwindow *window, double &x, double &y) {
  glfwGetCursorPos(window, &x, &y);
  int fbWidth, fbHeight;
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
  int winWidth, winHeight;
  glfwGetWindowSize(window, &winWidth, &winHeight);

  float scaleX = (float)fbWidth / winWidth;
  float scaleY = (float)fbHeight / winHeight;

  x *= scaleX;
  y *= scaleY;

  float normalized_x = (float)x / fbWidth;
  normalized_x = normalized_x * 2.0f - 1.0f;

  float normalized_y = (float)y / fbHeight;
  normalized_y = -(normalized_y * 2.0f - 1.0f);
  x = normalized_x;
  y = normalized_y;
}

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    double x, y;
    convertToNDC(window, x, y);
    circles.push_back(Circle(x, y, 0.1));
  }
}

void cursorPosCallback(GLFWwindow *window, double x, double y) {
  stringstream ss;
  ss << "X: " << (int)x << ", Y: " << (int)y;
  mouseCoords = ss.str();
}

void renderText() {
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  int w, h;
  glfwGetFramebufferSize(glfwGetCurrentContext(), &w, &h);
  glOrtho(0, w, h, 0, -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor3f(1.0f, 1.0f, 1.0f);

  float textH = font->FaceSize();
  float padding = 10.0f;
  float xPos = w - font->Advance(mouseCoords.c_str()) - padding;
  float yPos = h - textH - padding;

  glRasterPos2f(xPos, yPos);
  font->Render(mouseCoords.c_str());

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glPopAttrib();
}

void linearMotion(Circle &circle, double dt) {
  circle.prevX = circle.x;
  circle.prevY = circle.y;

  vec2 deltaV = vec2_mul(circle.acc, dt);
  circle.velocity = vec2_add(circle.velocity, deltaV);

  capVelocity(circle.velocity);

  vec2 deltaP = vec2_add(vec2_mul(circle.velocity, dt),
                         vec2_mul(circle.acc, 0.5 * dt * dt));
  circle.position = vec2_add(circle.position, deltaP);
  circle.x = circle.position.x;
  circle.y = circle.position.y;

  if (circle.y <= -1.0 + circle.radius) {
    circle.y = -1.0 + circle.radius;
    circle.position.y = -1.0 + circle.radius;
    circle.velocity.y = -RESTITUTION * circle.velocity.y;
    if (fabs(circle.velocity.y) < MIN_VELOCITY) {
      circle.velocity.y = 0;
    }
  }

  if (circle.y >= 1.0 - circle.radius) {
    circle.y = 1.0 - circle.radius;
    circle.position.y = 1.0 - circle.radius;
    circle.velocity.y = -RESTITUTION * circle.velocity.y;
    if (fabs(circle.velocity.y) < MIN_VELOCITY) {
      circle.velocity.y = 0;
    }
  }

  if (circle.x <= -1.0 + circle.radius) {
    circle.x = -1.0 + circle.radius;
    circle.position.x = -1.0 + circle.radius;
    circle.velocity.x = -RESTITUTION * circle.velocity.x;
    if (fabs(circle.velocity.x) < MIN_VELOCITY) {
      circle.velocity.x = 0;
    }
  }

  if (circle.x >= 1.0 - circle.radius) {
    circle.x = 1.0 - circle.radius;
    circle.position.x = 1.0 - circle.radius;
    circle.velocity.x = -RESTITUTION * circle.velocity.x;
    if (fabs(circle.velocity.x) < MIN_VELOCITY) {
      circle.velocity.x = 0;
    }
  }

  capVelocity(circle.velocity);
}

void renderCircles() {
  int width, height;
  glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);

  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glColor3f(1.0f, 1.0f, 1.0f);

  double subDt = MOTION_TIME / PHYSICS_STEPS;
  for (int step = 0; step < PHYSICS_STEPS; step++) {
    for (auto &circle : circles) {
      linearMotion(circle, subDt);
    }
    checkCollisions();
  }

  for (auto &circle : circles) {
    drawCircle(circle, 32);
  }
}

int main() {
  if (!glfwInit()) {
    cerr << "Failed to initialize GLFW" << endl;
    return -1;
  }

  glfwSetErrorCallback(errorCallback);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(800, 600, "Circle Physics", nullptr, nullptr);
  if (!window) {
    cerr << "Failed to create window" << endl;
    glfwTerminate();
    return -1;
  }

  glfwSetMouseButtonCallback(window, mouseButtonCallback);
  glfwSetCursorPosCallback(window, cursorPosCallback);
  glfwMakeContextCurrent(window);

  font = new FTGLPixmapFont("/System/Library/Fonts/Helvetica.ttc");
  if (font->Error()) {
    cerr << "Failed to load font" << endl;
    delete font;
    glfwTerminate();
    return -1;
  }
  font->FaceSize(36);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);
    renderCircles();
    renderText();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  delete font;
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
