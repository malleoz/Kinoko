#include "ObjectDirector.hh"

#include "game/field/obj/ObjectRegistry.hh"

#include "game/system/CourseMap.hh"

namespace Field {

const ObjectFlowTable &ObjectDirector::flowTable() const {
    return m_flowTable;
}

/// @addr{0x8082A784}
ObjectDirector *ObjectDirector::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = new ObjectDirector;

    s_instance->createObjects();

    return s_instance;
}

/// @addr{0x8082A824}
void ObjectDirector::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    delete instance;
}

ObjectDirector *ObjectDirector::Instance() {
    return s_instance;
}

/// @addr{0x8082A38C}
ObjectDirector::ObjectDirector() : m_flowTable("ObjFlow.bin") {}

/// @addr{0x8082A694}
ObjectDirector::~ObjectDirector() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("ObjectDirector instance not explicitly handled!");
    }

    for (auto *&obj : m_objects) {
        delete obj;
    }
}

/// @addr{0x80826E8C}
void ObjectDirector::createObjects() {
    const auto *courseMap = System::CourseMap::Instance();
    size_t objectCount = courseMap->getGeoObjCount();
    m_objects.reserve(objectCount);

    for (size_t i = 0; i < objectCount; ++i) {
        const auto *pObj = courseMap->getGeoObj(i);
        ASSERT(pObj);

        // Assume one player - if the presence flag isn't set, don't construct it
        if (!(pObj->presenceFlag() & 1)) {
            continue;
        }

        // Prevent construction of objects with disabled or no collision
        if (IsObjectBlacklisted(pObj->id())) {
            continue;
        }

        ObjectBase *object = createObject(*pObj);
        object->init();
        m_objects.push_back(object);
    }
}

/// @addr{0x80821E14}
ObjectBase *ObjectDirector::createObject(const System::MapdataGeoObj &params) {
    ObjectId id = static_cast<ObjectId>(params.id());
    switch (id) {
    case ObjectId::DokanSFC:
        return new ObjectDokan(params);
    case ObjectId::OilSFC:
        return new ObjectOilSFC(params);
    default:
        return new ObjectNoImpl(params);
    }
}

ObjectDirector *ObjectDirector::s_instance = nullptr; ///< @addr{0x809C4330}

} // namespace Field
