#include "trackball.h"

#include "glcheck.h"

#include <glm/gtx/orthonormalize.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

Trackball::Trackball() {
    m_fScale[0] = 0.05f;
    m_fScale[1] = 0.05f;
    m_fScale[2] = 0.05f;

    m_fScale[3] = 0.003f;
    m_fScale[4] = 0.003f;
    m_fScale[5] = 0.003f;

    m_fScale[6] = 0.0f;

    m_transAxis[0] = glm::vec3(1, 0, 0);
    m_transAxis[1] = glm::vec3(0, 1, 0);
    m_transAxis[2] = glm::vec3(0, 0, 1);
}

Trackball::~Trackball() {}

void Trackball::setViewState(const glm::mat4x4 &m) {
    m_invView = glm::mat4x4(glm::orthonormalize(glm::mat3x3(m)));

    m_invView = glm::inverse(m_invView);

    m_transAxis[0] = m_invView * glm::vec4(1, 0, 0, 0);
    m_transAxis[1] = m_invView * glm::vec4(0, 1, 0, 0);
    m_transAxis[2] = m_invView * glm::vec4(0, 0, 1, 0);
}

void Trackball::grab(const int &x, const int &y) {
    m_last = m_track;

    glm::mat4x4 m = glm::inverse(m_invView);

    m_rotation = m;

    m_iLastPt[0] = x;
    m_iLastPt[1] = y;
}

void Trackball::update(const int &x, const int &y) {
    process(x - m_iLastPt[0], m_iXManip);
    process(y - m_iLastPt[1], m_iYManip);

    m_iLastPt[0] = x;
    m_iLastPt[1] = y;

    m_track = m_last * m_rotation * m_invView;
}

void Trackball::release() {
    // does nothing?
}

void Trackball::identity() {
    m_track   = glm::mat4x4(1);
    m_last    = glm::mat4x4(1);
    m_invView = glm::mat4x4(1);

    m_translate = glm::vec3(0.f, 0.f, 0.f);
}

float Trackball::getMultiplier(const Manipulation &m) const {
    float ret = 0.f;
    switch(m) {
        case Manipulation::Manipulation_Pitch:
            ret = m_fScale[1];
            break;

        case Manipulation::Manipulation_Yaw:
            ret = m_fScale[2];
            break;

        case Manipulation::Manipulation_Roll:
            ret = m_fScale[3];
            break;

        case Manipulation::Manipulation_TransX:
            ret = m_fScale[4];
            break;

        case Manipulation::Manipulation_TransY:
            ret = m_fScale[5];
            break;

        case Manipulation::Manipulation_TransZ:
            ret = m_fScale[6];
            break;
    }

    return ret;
}

void Trackball::setMultiplier(const Manipulation &m, const float &f) {
    switch(m) {
        case Manipulation::Manipulation_Pitch:
            m_fScale[1] = f;
            break;

        case Manipulation::Manipulation_Yaw:
            m_fScale[2] = f;
            break;

        case Manipulation::Manipulation_Roll:
            m_fScale[3] = f;
            break;

        case Manipulation::Manipulation_TransX:
            m_fScale[4] = f;
            break;

        case Manipulation::Manipulation_TransY:
            m_fScale[5] = f;
            break;

        case Manipulation::Manipulation_TransZ:
            m_fScale[6] = f;
            break;
    }
}

void Trackball::process(const int &delta, Manipulation m) {
    switch(m) {
        case Manipulation::Manipulation_Pitch:
            m_rotation = glm::rotate(m_rotation, -float(delta) * m_fScale[1], glm::vec3(1.f, 0.f, 0.f));
            break;

        case Manipulation::Manipulation_Yaw:
            m_rotation = glm::rotate(m_rotation, float(delta) * m_fScale[2], glm::vec3(0.0, 1.0, 0.0));
            break;

        case Manipulation::Manipulation_Roll:
            m_rotation = glm::rotate(m_rotation, -float(delta) * m_fScale[3], glm::vec3(0.0, 0.0, 1.0));
            break;

        case Manipulation::Manipulation_TransX:
            m_translate += float(delta) * m_fScale[4] * m_transAxis[0];
            break;

        case Manipulation::Manipulation_TransY:
            m_translate += float(delta) * m_fScale[5] * m_transAxis[1];
            break;

        case Manipulation::Manipulation_TransZ:
            m_translate += float(delta) * m_fScale[6] * m_transAxis[2];
            break;

        default:
            break;
    }
}

void Trackball::glMult() {
    //glm::mat4x4 m(1.0);

    //m = glm::translate(m, m_translate);

    //glMultMatrixf(glm::value_ptr(m));
    //check_error();
}
