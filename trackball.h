#ifndef _Trackball_h_
#define _Trackball_h_

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

enum class Manipulation {
    Manipulation_None = 0,
    Manipulation_Pitch,
    Manipulation_Yaw,
    Manipulation_Roll,
    Manipulation_TransX,
    Manipulation_TransY,
    Manipulation_TransZ
};

class Trackball {
  public:
    Trackball();
    ~Trackball();

    void setViewState(const glm::mat4x4 &m);

    inline void setManipulation(Manipulation xm, Manipulation ym) {
        m_iXManip = xm;
        m_iYManip = ym;
    }
    inline Manipulation getXManipulator() const { return m_iXManip; }
    inline Manipulation getYManipulator() const { return m_iYManip; }

    void grab(const int &x, const int &y);
    void update(const int &x, const int &y);
    void release();

    void identity();

    float getMultiplier(const Manipulation &m) const;
    void setMultiplier(const Manipulation &m, const float &f);

    void glMult();

    inline glm::mat4x4 getTransform() const { return glm::translate(m_track, m_translate); }

    inline void setTranslate(const glm::vec3 &t) { m_translate = t; }
    inline glm::vec3 getTranslate() const { return m_translate; }

  protected:
    void process(const int &delta, Manipulation m);

    int m_iLastPt[2];
    glm::mat4x4 m_rotation;
    glm::vec3 m_translate;
    glm::vec3 m_transAxis[3];

    glm::mat4x4 m_last, m_track, m_invView;

    Manipulation m_iXManip, m_iYManip;

    float m_fScale[7];
};

#endif
