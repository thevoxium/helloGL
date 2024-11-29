#define GL_SILENCE_DEPRECATION
#include <FTGL/ftgl.h>
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

struct Circle {
  double x, y, radius;
};

FTGLPixmapFont *font = nullptr;
string mouseCoords = "X: 0, Y: 0";
vector<Circle> circles;

void errorCallback(int error, const char *message) {
  cerr << "GLFW Error " << error << ": " << message << endl;
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

  float normalized_x = (float)x / fbWidth;   // First normalize to [0,1]
  normalized_x = normalized_x * 2.0f - 1.0f; // Then convert to [-1,1]

  float normalized_y = (float)y / fbHeight;
  normalized_y = -(normalized_y * 2.0f - 1.0f); // Flip Y and convert to [-1,1]
  x = normalized_x;
  y = normalized_y;
}

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    double x, y;
    convertToNDC(window, x, y);
    Circle newCircle{(float)x, (float)y, 0.1};
    circles.push_back(newCircle);
  }
}
void cursorPosCallback(GLFWwindow *window, double x, double y) {
  stringstream ss;
  ss << "X: " << (int)x << ", Y: " << (int)y;
  mouseCoords = ss.str();
}

void renderText() {
  // Switch to compatibility profile state
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

  // Enable texturing and blending for text
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

  // Restore previous state
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glPopAttrib();
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

  glColor3f(1.0f, 0.0f, 0.0f);
  for (const auto &circle : circles) {
    drawCircle(circle, 32);
  }
}
int main() {
  if (!glfwInit()) {
    cerr << "Failed to initialize GLFW" << endl;
    return -1;
  }

  glfwSetErrorCallback(errorCallback);

  // Request compatibility profile instead of core profile
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(800, 600, "Mouse Coordinates", nullptr, nullptr);
  if (!window) {
    cerr << "Failed to create window" << endl;
    glfwTerminate();
    return -1;
  }

  glfwSetMouseButtonCallback(window, mouseButtonCallback);
  glfwSetCursorPosCallback(window, cursorPosCallback);
  glfwMakeContextCurrent(window);

  // Initialize font
  font = new FTGLPixmapFont("/System/Library/Fonts/Helvetica.ttc");
  if (font->Error()) {
    cerr << "Failed to load font" << endl;
    delete font;
    glfwTerminate();
    return -1;
  }
  font->FaceSize(36);

  // Set clear color
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