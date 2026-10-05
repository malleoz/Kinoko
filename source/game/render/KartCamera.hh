#pragma once

#include "game/kart/KartMove.hh"

namespace Kinoko::Render {

/// @brief Tracks the current state of a camera (either the front or backwards player cameras)
class KartCameraState {
    /// @brief Grants access to the @ref KartCamera class so it can manipulate the camera state
    /// directly
    friend class KartCamera;

public:
    /// @brief Constructor
    /// @details Zero-intiailzies all members.
    KartCameraState() {
        m_pos.setZero();
        m_bigAirHeight = 0.0f;
        m_downPitchRatio = 0.0f;
        m_prevPos.setZero();
        m_dist = 0.0f;
        m_bigAirFallPitch = 0.0f;
        m_targetPos.setZero();
    }

    /// @brief Default destructor
    ~KartCameraState() = default;

    /// @addr{0x805A1C3C}
    /// @brief Initializes the camera state to default values
    /// @details Resets @ref m_dist and @ref m_bigAirFallPitch to zero.
    void init() {
        m_dist = 0.0f;
        m_bigAirFallPitch = 0.0f;
    }

private:
    EGG::Vector3f m_pos;       ///< The current position of the camera
    f32 m_bigAirHeight;        ///< Additional camera height applied after 20 frames of airtime
    f32 m_downPitchRatio;      ///< Smoothed ratio scaling the camera's downward pitch angle
    EGG::Vector3f m_prevPos;   ///< The position of the camera on the previous frame
    f32 m_dist;                ///< Orbit distance from @ref m_targetPos
    f32 m_bigAirFallPitch;     ///< Height applied once you start falling after 20 frames of airtime
    EGG::Vector3f m_targetPos; ///< The position the camera looks towards
};

/// @brief Manager class for the forward and backwards cameras.
/// @details Responsible for setting the camera state and performing camera collision checks.
/// @note The camera physics need to be implemented in Kinoko because the camera performs collision
/// checks during the race via @ref calcCollision(), which can result in an @ref Field::ObjColMgr
/// transformation matrix update. This can cause desyncs on DS Delfino Square if not implemented.
/// @warning Kinoko assumes the use of the 16:9 camera. It is possible for a time trial
/// desync to occur due to the difference in camera distance between 4:3 and 16:9. This
/// varying distance can cause an ObjectKCL transformation matrix to be updated for one
/// camera's collision check but not the other. As far as we know, this can only occur on
/// the DS Delfino Square @ref Field::ObjectTownBridge, and there are no naturally occurring
/// desyncs of this type.
class KartCamera {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    /// @addr{0x805A2034}
    /// @brief Initializes the camera with the default parameters and position
    /// @details Gets the camera parameters from the first kart object (since we assume only one
    /// player in Kinoko) and stores the pointer to @ref m_camParams. Then calls @ref initPos() to
    /// initialize the camera's position.
    void init() {
        auto *param = Kart::KartObjectManager::Instance()->object(0)->param();
        m_camParams = &param->camera();

        initPos();
    }

    void calc();

    /// @brief Creates the singleton instance of the @ref KartCamera
    /// @return A pointer to the newly created @ref KartCamera instance
    /// @pre The singleton instance must not already exist.
    static KartCamera *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<KartCamera>();
        return s_instance;
    }

    /// @brief Destroys the singleton instance of the @ref KartCamera
    /// @pre The singleton instance must exist.
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref KartCamera
    /// @return A pointer to the singleton instance of the @ref KartCamera
    [[nodiscard]] static KartCamera *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    /// @addr{0x805A1D10}
    /// @brief Private constructor
    /// @details Sets @ref m_hopPosY and @ref m_forward to zero and initializes @ref m_camParams to
    /// `nullptr`.
    KartCamera() : m_hopPosY(0), m_forward(EGG::Vector3f::zero), m_camParams(nullptr) {}

    /// @addr{0x805A8F7C}
    /// @brief Private default destructor
    ~KartCamera() = default;

    /// @addr{0x805A2B84}
    /// @brief Interpolates the camera's orientation based on the kart's smoothed forward vector
    /// @param t Interpolation factor
    /// @param proxy Pointer to the kart object proxy
    void calcForward(f32 t, const Kart::KartObjectProxy *proxy) {
        m_forward = Interpolate(t, m_forward, proxy->move()->smoothedForward());
        m_right = EGG::Vector3f::ey.perpInPlane(m_forward, true);
    }

    void calcManualDriftOffset(const Kart::KartObjectProxy *proxy);
    void calcCamera(f32 param1, f32 param2, f32 param3, KartCameraState &state, bool isBackwards,
            const Kart::KartObjectProxy *proxy, const EGG::Vector3f &targetPos) const;
    void calcAirtime(KartCameraState &state, const Kart::KartObjectProxy *proxy) const;
    void initPos();

    void calcCollision(KartCameraState &state, bool isRear) const;

    /// @addr{0x805A2C34}
    /// @brief Linearly interpolates between two vectors
    /// @param t Interpolation factor
    /// @param v0 Starting vector
    /// @param v1 Ending vector
    /// @return The interpolated vector
    static EGG::Vector3f Interpolate(f32 t, const EGG::Vector3f &v0, const EGG::Vector3f &v1) {
        return v0 + (v1 - v0) * t;
    }

    f32 m_driftYaw;                                      ///< Rotation induced when drifting
    f32 m_hopPosY;                                       ///< Tracks the hop height of the vehicle
    EGG::Vector3f m_forward;                             ///< Forward direction of the camera
    EGG::Vector3f m_right;                               ///< Right direction of the camera
    const Kart::KartParam::KartCameraParam *m_camParams; ///< The camera parameters for the kart
    KartCameraState m_forwardCamera;                     ///< Forward camera state
    KartCameraState m_backwardCamera;                    ///< Rear camera state

    /// @brief Singleton instance of the @ref KartCamera
    /// @note In the base game, there is a @ref KartCamera instance for each player. Since we only
    /// implement one player for Kinoko, this can effectively become a singleton.
    static KartCamera *s_instance;
};

} // namespace Kinoko::Render
