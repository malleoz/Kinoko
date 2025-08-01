#pragma once

#include "host/KSystem.hh"

#include <egg/core/SceneManager.hh>

#include <game/system/RaceConfig.hh>

namespace System {

class KPadHostController;

}

class KHostSystem final : public KSystem {
public:
    void init() override;
    void calc() override;
    bool run() override;
    void parseOptions(int argc, char **argv) override;

    static KHostSystem *CreateInstance();
    static void DestroyInstance();

    static KHostSystem *Instance() {
        return static_cast<KHostSystem *>(s_instance);
    }

private:
    struct KRaceParams {
        Course course;
        Character character;
        Vehicle vehicle;
        bool driftIsAuto;
    };

    KHostSystem();
    KHostSystem(const KHostSystem &) = delete;
    KHostSystem(KHostSystem &&) = delete;
    ~KHostSystem() override;

    static Course PromptForCourse();
    static Character PromptForCharacter();
    static Vehicle PromptForVehicle();
    static bool PromptForAuto();
    void ValidateParams(const KRaceParams &params);

    static void OnInit(System::RaceConfig *config, void *arg);

    EGG::SceneManager *m_sceneMgr;
    KRaceParams m_params;

    const System::KPadHostController *m_controller;
};
