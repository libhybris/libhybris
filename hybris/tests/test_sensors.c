/*
 * Copyright (c) 2012 Simon Busch <morphis@gravedo.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <android-config.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <hardware/sensors.h>

int main(int argc, char **argv)
{
	struct hw_module_t *hwmod;
	struct sensors_poll_device_t *dev;

	hw_get_module(SENSORS_HARDWARE_MODULE_ID, (const hw_module_t**) &hwmod);
	assert(hwmod != NULL);

	if (sensors_open(hwmod, &dev) < 0) {
		printf("ERROR: failed to open sensors device\n");
		exit(1);
	}

        printf("Hardware module ID: %s\n", hwmod->id);
        printf("Hardware module Name: %s\n", hwmod->name);
        printf("Hardware module Author: %s\n", hwmod->author);
        printf("Hardware module API version: 0x%x\n", hwmod->module_api_version);
        printf("Hardware HAL API version: 0x%x\n", hwmod->hal_api_version);
        printf("Poll device version: 0x%x\n", dev->common.version);

        printf("API VERSION 0.1 (legacy): 0x%x\n", HARDWARE_MODULE_API_VERSION(0, 1));
#ifdef SENSORS_DEVICE_API_VERSION_0_1
        printf("API VERSION 0.1: 0x%d\n", SENSORS_DEVICE_API_VERSION_0_1);
#endif
#ifdef SENSORS_DEVICE_API_VERSION_1_0
        printf("API VERSION 1.0: 0x%d\n", SENSORS_DEVICE_API_VERSION_1_0);
#endif
#ifdef SENSORS_DEVICE_API_VERSION_1_1
        printf("API VERSION 1.1: 0x%d\n", SENSORS_DEVICE_API_VERSION_1_1);
#endif

        struct sensors_module_t *smod = (struct sensors_module_t *)(hwmod);

        struct sensor_t const *sensors_list = NULL;
        int sensors = smod->get_sensors_list(smod, &sensors_list);
        printf("Got %d sensors\n", sensors);

        int i, j, res;
        for (i=0; i<sensors; i++) {
            struct sensor_t const *s = sensors_list + i;
            printf("=== Sensor %d ==\n", i);
            printf("Name: %s\n", s->name);
            printf("Vendor: %s\n", s->vendor);
            printf("Version: 0x%x\n", s->version);
            printf("Handle: 0x%x\n", s->handle);
            printf("Type: %d\n", s->type);
            printf("maxRange: %.f\n", s->maxRange);
            printf("resolution: %.f\n", s->resolution);
            printf("power: %.f mA\n", s->power);
            printf("minDelay: %d\n", s->minDelay);
            //printf("fifoReservedEventCount: %d\n", s->fifoReservedEventCount);
            //printf("fifoMaxEventCount: %d\n", s->fifoMaxEventCount);

            res = dev->setDelay(dev, s->handle, s->minDelay);
            if (res != 0) {
                printf("Could not set delay: %s\n", strerror(-res));
            }
            res = dev->activate(dev, s->handle, 1);
            if (res != 0) {
                printf("Could not activate sensor: %s\n", strerror(-res));
            } else {
                printf("Reading events\n");
                for (j=0; j<30; j++) {
                    sensors_event_t data;
                    printf("Polling... ");
                    fflush(stdout);
                    res = dev->poll(dev, &data, 1);
                    printf("got event\n");
                }
                res = dev->activate(dev, s->handle, 0);
                if (res != 0) {
                    printf("Could not deactivate sensor: %s\n", strerror(-res));
                }
            }

            printf("\n\n\n");
        }

	if (sensors_close(dev) < 0) {
		printf("ERROR: failed to close sensors device\n");
		exit(1);
	}

	return 0;
}

// vim:ts=4:sw=4:noexpandtab
