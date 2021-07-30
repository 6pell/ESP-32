#pragma once
#include "Module.h"

class InfraredModule : public Module {
    private:
    public:
        InfraredModule();
        virtual ~InfraredModule();
        // Module identification
        virtual const char*   getId()    { return INFRARED_MODULE; }
        virtual const char*   getName()  { return Messages::TITLE_INFRARED_TRANSMITTER; }
};