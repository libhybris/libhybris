/*
 * Copyright (c) 2013 Canonical Ltd
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
#include <memory.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <hardware/audio.h>
#include <hardware/hardware.h>

int main(int argc, char **argv)
{
	struct hw_module_t *hwmod = 0;
	struct audio_hw_device *audiohw;

	hw_get_module_by_class(AUDIO_HARDWARE_MODULE_ID,
#if defined(AUDIO_HARDWARE_MODULE_ID_PRIMARY)
					AUDIO_HARDWARE_MODULE_ID_PRIMARY,
#else
					"primary",
#endif
					(const hw_module_t**) &hwmod);
	assert(hwmod != NULL);

	assert(audio_hw_device_open(hwmod, &audiohw) == 0);
	assert(audiohw->init_check(audiohw) == 0);
	printf("Audio Hardware Interface initialized.\n");

#if (ANDROID_VERSION_MAJOR == 4 && ANDROID_VERSION_MINOR >= 1) || (ANDROID_VERSION_MAJOR >= 5)
	if (audiohw->get_master_volume) {
		float volume;
		audiohw->get_master_volume(audiohw, &volume);
		printf("Master Volume: %f\n", volume);
	}
#endif

#if (ANDROID_VERSION_MAJOR == 4 && ANDROID_VERSION_MINOR >= 2) || (ANDROID_VERSION_MAJOR >= 5)
	if (audiohw->get_master_mute) {
		bool mute;
		audiohw->get_master_mute(audiohw, &mute);
		printf("Master Mute: %d\n", mute);
	}
#endif

	audio_hw_device_close(audiohw);

	return 0;
}

// vim:ts=4:sw=4:noexpandtab
