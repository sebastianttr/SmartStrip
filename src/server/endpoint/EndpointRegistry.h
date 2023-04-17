//
// Created by Sebastian on 17.04.23.
//

#ifndef SMARTSTRIP_ENDPOINTREGISTRY_H
#define SMARTSTRIP_ENDPOINTREGISTRY_H

#include "HTTPEndpoint.h"

class EndpointRegistry {
private:

public:
    static void registerEndpoints(HTTPEndpoint& httpEndpoint);
};


#endif //SMARTSTRIP_ENDPOINTREGISTRY_H
