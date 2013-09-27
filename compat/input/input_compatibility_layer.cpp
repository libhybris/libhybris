/*
 * Copyright (C) 2013 Canonical Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 *              Ricardo Salveti de Araujo <ricardo.salveti@canonical.com>
 */

#include <hybris/input/input_stack_compatibility_layer.h>

#include "InputListener.h"
#include "InputReader.h"
#include "PointerController.h"
#include "SpriteController.h"
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>

#undef LOG_TAG
#define LOG_TAG "InputStackCompatibilityLayer"
#include <utils/Log.h>

namespace
{
static bool enable_verbose_function_reporting = false;
}

#define REPORT_FUNCTION() ALOGV("%s\n", __PRETTY_FUNCTION__);

namespace
{

class DefaultPointerControllerPolicy : public android::PointerControllerPolicyInterface
{
public:
	static const size_t bitmap_width = 64;
	static const size_t bitmap_height = 64;

	DefaultPointerControllerPolicy()
	{
		bitmap.setConfig(
				SkBitmap::kARGB_8888_Config,
				bitmap_width,
				bitmap_height);
		bitmap.allocPixels();

		// Icon for spot touches
		bitmap.eraseARGB(125, 0, 255, 0);
		spotTouchIcon = android::SpriteIcon(
				bitmap,
				bitmap_width/2,
				bitmap_height/2);

		// Icon for anchor touches
		bitmap.eraseARGB(125, 0, 0, 255);
		spotAnchorIcon = android::SpriteIcon(
				bitmap,
				bitmap_width/2,
				bitmap_height/2);

		// Icon for hovering touches
		bitmap.eraseARGB(125, 255, 0, 0);
		spotHoverIcon = android::SpriteIcon(
				bitmap,
				bitmap_width/2,
				bitmap_height/2);
	}

	void loadPointerResources(android::PointerResources* outResources)
	{
		outResources->spotHover = spotHoverIcon.copy();
		outResources->spotTouch = spotTouchIcon.copy();
		outResources->spotAnchor = spotAnchorIcon.copy();
	}

	android::SpriteIcon spotHoverIcon;
	android::SpriteIcon spotTouchIcon;
	android::SpriteIcon spotAnchorIcon;
	SkBitmap bitmap;
};

class DefaultInputReaderPolicyInterface : public android::InputReaderPolicyInterface
{
public:
	static const int32_t internal_display_id = android::ISurfaceComposer::eDisplayIdMain;
	static const int32_t external_display_id = android::ISurfaceComposer::eDisplayIdHdmi;

	DefaultInputReaderPolicyInterface(
			InputStackConfiguration* configuration,
			const android::sp<android::Looper>& looper)
			: looper(looper),
		default_layer_for_touch_point_visualization(configuration->default_layer_for_touch_point_visualization),
		input_area_width(configuration->input_area_width),
		input_area_height(configuration->input_area_height)
	{
		default_configuration.showTouches = configuration->enable_touch_point_visualization;

		android::DisplayViewport viewport;
		viewport.setNonDisplayViewport(input_area_width, input_area_height);
		viewport.displayId = android::ISurfaceComposer::eDisplayIdMain;
		default_configuration.setDisplayInfo(
				false, /* external */
				viewport);
	}

	void getReaderConfiguration(android::InputReaderConfiguration* outConfig)
	{
		*outConfig = default_configuration;
	}

	android::sp<android::PointerControllerInterface> obtainPointerController(int32_t deviceId)
	{
		(void) deviceId;

		android::sp<android::SpriteController> sprite_controller(
				new android::SpriteController(
					looper,
					default_layer_for_touch_point_visualization));
		android::sp<android::PointerController> pointer_controller(
				new android::PointerController(
					android::sp<DefaultPointerControllerPolicy>(new DefaultPointerControllerPolicy()),
					looper,
					sprite_controller));
		pointer_controller->setPresentation(
				android::PointerControllerInterface::PRESENTATION_SPOT);

		pointer_controller->setDisplayViewport(input_area_width, input_area_height, 0);
		return pointer_controller;
	}

	virtual void notifyInputDevicesChanged(const android::Vector<android::InputDeviceInfo>& inputDevices) {
		mInputDevices = inputDevices;
	}

	virtual android::sp<android::KeyCharacterMap> getKeyboardLayoutOverlay(const android::String8& inputDeviceDescriptor) {
		return NULL;
	}

	virtual android::String8 getDeviceAlias(const android::InputDeviceIdentifier& identifier) {
		return android::String8::empty();
	}

private:
	android::sp<android::Looper> looper;
	int default_layer_for_touch_point_visualization;
	android::InputReaderConfiguration default_configuration;
	android::Vector<android::InputDeviceInfo> mInputDevices;
	int input_area_width;
	int input_area_height;
};

class ExportedInputListener : public android::InputListenerInterface
{
public:
	ExportedInputListener(AndroidEventListener* external_listener) : external_listener(external_listener)
	{
	}

	void notifyConfigurationChanged(const android::NotifyConfigurationChangedArgs* args)
	{
		REPORT_FUNCTION();
		(void) args;
	}

	void notifyKey(const android::NotifyKeyArgs* args)
	{
		REPORT_FUNCTION();

		current_event.type = KEY_EVENT_TYPE;
		current_event.device_id = args->deviceId;
		current_event.source_id = args->source;
		current_event.action = args->action;
		current_event.flags = args->flags;
		current_event.meta_state = args->metaState;

		current_event.details.key.key_code = args->keyCode;
		current_event.details.key.scan_code = args->scanCode;
		current_event.details.key.down_time = args->downTime;
		current_event.details.key.event_time = args->eventTime;

		current_event.details.key.is_system_key = false;

		external_listener->on_new_event(&current_event, external_listener->context);
	}

	void notifyMotion(const android::NotifyMotionArgs* args)
	{
		REPORT_FUNCTION();

		current_event.type = MOTION_EVENT_TYPE;
		current_event.device_id = args->deviceId;
		current_event.source_id = args->source;
		current_event.action = args->action;
		current_event.flags = args->flags;
		current_event.meta_state = args->metaState;

		current_event.details.motion.button_state = args->buttonState;
		current_event.details.motion.down_time = args->downTime;
		current_event.details.motion.event_time = args->eventTime;
		current_event.details.motion.edge_flags = args->edgeFlags;
		current_event.details.motion.x_precision = args->xPrecision;
		current_event.details.motion.y_precision = args->yPrecision;
		current_event.details.motion.pointer_count = args->pointerCount;

		for (unsigned int i = 0; i < current_event.details.motion.pointer_count; i++) {
			current_event.details.motion.pointer_coordinates[i].id = args->pointerProperties[i].id;
			current_event.details.motion.pointer_coordinates[i].x
				= current_event.details.motion.pointer_coordinates[i].raw_x
				= args->pointerCoords[i].getX();
			current_event.details.motion.pointer_coordinates[i].y
				= current_event.details.motion.pointer_coordinates[i].raw_y
				= args->pointerCoords[i].getY();
			current_event.details.motion.pointer_coordinates[i].touch_major
				= args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR);
			current_event.details.motion.pointer_coordinates[i].touch_minor
				= args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR);
			current_event.details.motion.pointer_coordinates[i].pressure
				= args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_PRESSURE);
			current_event.details.motion.pointer_coordinates[i].size
				= args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_SIZE);
			current_event.details.motion.pointer_coordinates[i].orientation
				= args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_ORIENTATION);

		}

		external_listener->on_new_event(&current_event, external_listener->context);
	}

	void notifySwitch(const android::NotifySwitchArgs* args)
	{
		REPORT_FUNCTION();
		current_event.type = HW_SWITCH_EVENT_TYPE;

		current_event.details.hw_switch.event_time = args->eventTime;
		current_event.details.hw_switch.policy_flags = args->policyFlags;
		current_event.details.hw_switch.switch_values = args->switchValues;
		current_event.details.hw_switch.switch_mask = args->switchMask;

		external_listener->on_new_event(&current_event, external_listener->context);
	}

	void notifyDeviceReset(const android::NotifyDeviceResetArgs* args)
	{
		REPORT_FUNCTION();
		(void) args;
	}

private:
	AndroidEventListener* external_listener;
	Event current_event;
};

class LooperThread : public android::Thread
{
public:
	static const int default_poll_timeout_ms = -1;

	LooperThread(const android::sp<android::Looper>& looper) : looper(looper)
	{
	}

private:
	bool threadLoop()
	{
		if (ALOOPER_POLL_ERROR == looper->pollAll(default_poll_timeout_ms))
			return false;
		return true;
	}

	android::sp<android::Looper> looper;
};

struct State : public android::RefBase
{
	State(AndroidEventListener* listener,
			InputStackConfiguration* configuration)
			: looper(new android::Looper(false)),
			looper_thread(new LooperThread(looper)),
			event_hub(new android::EventHub()),
			input_reader_policy(new DefaultInputReaderPolicyInterface(configuration, looper)),
			input_listener(new ExportedInputListener(listener)),
			input_reader(new android::InputReader(
						event_hub,
						input_reader_policy,
						input_listener)),
			input_reader_thread(new android::InputReaderThread(input_reader))
	{
	}

	~State()
	{
		input_reader_thread->requestExit();
	}

	android::sp<android::Looper> looper;
	android::sp<LooperThread> looper_thread;

	android::sp<android::EventHubInterface> event_hub;
	android::sp<android::InputReaderPolicyInterface> input_reader_policy;
	android::sp<android::InputListenerInterface> input_listener;
	android::sp<android::InputReaderInterface> input_reader;
	android::sp<android::InputReaderThread> input_reader_thread;

	android::Condition wait_condition;
	android::Mutex wait_guard;
};

android::sp<State> global_state;

}

void android_input_stack_initialize(AndroidEventListener* listener, InputStackConfiguration* config)
{
	global_state = new State(listener, config);
}

void android_input_stack_loop_once()
{
	global_state->input_reader->loopOnce();
}

void android_input_stack_start()
{
	global_state->input_reader_thread->run();
	global_state->looper_thread->run();
}

void android_input_stack_start_waiting_for_flag(bool* flag)
{
	global_state->input_reader_thread->run();
	global_state->looper_thread->run();

	while (!*flag) {
		global_state->wait_condition.waitRelative(
				global_state->wait_guard,
				10 * 1000 * 1000);
	}
}

void android_input_stack_stop()
{
	global_state->input_reader_thread->requestExit();
}

void android_input_stack_shutdown()
{
	global_state = NULL;
}
