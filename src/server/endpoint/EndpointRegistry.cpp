//
// Created by Sebastian on 17.04.23.
//

#include "EndpointRegistry.h"

void EndpointRegistry::registerEndpoints(HTTPEndpoint& httpEndpoint) {
    int i= 0;
    Serial.println("Registering endpoint 1");
    httpEndpoint.onGet("/brightness", [=](http_request httpRequest) -> response {

        return {
                .statusCode = 200,
                .contentType = "text/plain",
                .content = "Setting brightness."
        };
    });


    Serial.println("Returning endpoint");
}
