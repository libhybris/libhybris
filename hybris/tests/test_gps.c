/*
 * Copyright (c) 2013 Jolla Mobile oy
 * Author: Philippe De Swert <philippe.deswert@jollamobile.com>
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

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <getopt.h>
#include <string.h>

#include <android/hardware/gps.h>

const GpsInterface* Gps = NULL;
const AGpsInterface* AGps = NULL;
const AGpsRilInterface* AGpsRil = NULL;
const GpsNiInterface *GpsNi = NULL;
const GpsXtraInterface *GpsExtra = NULL;

static const GpsInterface* get_gps_interface()
{
  int error;
  hw_module_t* module;
  const GpsInterface* interface = NULL;
  struct gps_device_t *device;

  error = hw_get_module(GPS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);

  if (!error)
  {
     error = module->methods->open(module, GPS_HARDWARE_MODULE_ID, (struct hw_device_t **) &device);
     fprintf(stdout, "*** device info\n id = %s\n name = %s\n author = %s\n",
     module->id, module->name, module->author);

     if (!error)
     {
       interface = device->get_gps_interface(device);
     }
  }
  else
  {
	fprintf(stdout, "*** GPS interface not found :\(\n Bye! \n");
	exit(1);
  }

  return interface;
}

static const AGpsInterface* get_agps_interface(const GpsInterface *gps)
{
  const AGpsInterface* interface = NULL;

  if (gps)
  {
    interface = (const AGpsInterface*)gps->get_extension(AGPS_INTERFACE);
  }
  return interface;
}

static const AGpsRilInterface* get_agps_ril_interface(const GpsInterface *gps)
{
  const AGpsRilInterface* interface = NULL;

  if (gps)
  {
    interface = (const AGpsRilInterface*)gps->get_extension(AGPS_RIL_INTERFACE);
  }
  return interface;
}

static const GpsNiInterface* get_gps_ni_interface(const GpsInterface *gps)
{
  const GpsNiInterface* interface = NULL;

  if(gps)
  {
    interface = (const GpsNiInterface*)gps->get_extension(GPS_NI_INTERFACE);
  }
  return interface;
}

static const GpsDebugInterface* get_gps_debug_interface(const GpsInterface *gps)
{
  const GpsDebugInterface* interface = NULL;

  if(gps)
  {
    interface = (const GpsDebugInterface*)gps->get_extension(GPS_DEBUG_INTERFACE);
  }
  return interface;
}

static const GpsXtraInterface* get_gps_extra_interface(const GpsInterface *gps)
{
  const GpsXtraInterface* interface = NULL;

  if(gps)
  {
    interface = (const GpsXtraInterface*)gps->get_extension(GPS_XTRA_INTERFACE);
  }
  return interface;
}

static void location_callback(GpsLocation* location)
{
  fprintf(stdout, "*** location callback\n");
  fprintf(stdout, "flags:\t%d\n", location->flags);
  fprintf(stdout, "latitude: \t%lf\n", location->latitude);
  fprintf(stdout, "longtide: \t%lf\n", location->longitude);
  fprintf(stdout, "accuracy:\t%f\n", location->accuracy);
  fprintf(stdout, "utc: \t%ld\n", (long)location->timestamp);
}

static void status_callback(GpsStatus* status)
{
  fprintf(stdout, "*** status callback\n");

  switch (status->status)
  {
    case GPS_STATUS_NONE:
	fprintf(stdout, "*** no gps\n");
	break;
    case GPS_STATUS_SESSION_BEGIN:
	fprintf(stdout, "*** session begin\n");
	break;
    case GPS_STATUS_SESSION_END:
	fprintf(stdout, "*** session end\n");
	break;
    case GPS_STATUS_ENGINE_ON:
	fprintf(stdout, "*** engine on\n");
	break;
    case GPS_STATUS_ENGINE_OFF:
	fprintf(stdout, "*** engine off\n");
	break;
    default:
	fprintf(stdout, "*** unknown status\n");
  }
}

static void sv_status_callback(GpsSvStatus* sv_info)
{
/* Too much useless info
  fprintf(stdout, "*** sv status\n");
  fprintf(stdout, "sv_size:\t%d\n", sv_info->size);
*/
}

static void nmea_callback(GpsUtcTime timestamp, const char* nmea, int length)
{
	char buf[83];
  fprintf(stdout, "*** nmea info\n");
  fprintf(stdout, "timestamp:\t%ld\n", (long)timestamp);
  /* NMEA sentences can only be between 11 ($TTFFF*CC\r\n) and 82 characters long */
  if (length > 10 && length < 83) {
    strncpy(buf, nmea, length);
    buf[length] = '\0';
    fprintf(stdout, "nmea (%d): \t%s\n", length, buf);
  } else {
    fprintf(stdout, "Invalid nmea data\n");
  }
}

static void set_capabilities_callback(uint32_t capabilities)
{
  fprintf(stdout, "*** set capabilities\n");
  fprintf(stdout, "capability is %.8x\n", capabilities);
  /* do nothing */
}

static void acquire_wakelock_callback()
{
  fprintf(stdout, "*** acquire wakelock\n");
  /* do nothing */
}

static void release_wakelock_callback()
{
  fprintf(stdout, "*** release wakelock\n");
  /* do nothing */
}

static pthread_t create_thread_callback(const char* name, void (*start)(void *), void* arg)
{
  pthread_t thread_id;
  pthread_attr_t attr;
  int error = 0;

  error = pthread_attr_init(&attr);
  error = pthread_create(&thread_id, &attr, (void*(*)(void*))start, arg);

  if(error != 0)
	return 0;

  return thread_id;
}


static void agps_handle_status_callback(AGpsStatus *status)
{
  switch (status->status)
  {
    case GPS_REQUEST_AGPS_DATA_CONN:
	fprintf(stdout, "*** data_conn_open\n");
	AGps->data_conn_open("internet");
	break;
    case GPS_RELEASE_AGPS_DATA_CONN:
	fprintf(stdout, "*** data_conn_closed\n");
	AGps->data_conn_closed();
	break;
  }
}

static void agps_ril_set_id_callback(uint32_t flags)
{
  fprintf(stdout, "*** set_id_cb\n");
  AGpsRil->set_set_id(AGPS_SETID_TYPE_IMSI, "000000000000000");
}

static void agps_ril_refloc_callback(uint32_t flags)
{
  fprintf(stdout, "*** refloc_cb\n");
  /* TODO : find out how to fill in location
  AGpsRefLocation location;
  AGpsRil->set_ref_location(&location, sizeof(location));
  */
}

static void ni_notify_callback (GpsNiNotification *notification)
{
  fprintf(stdout, "*** ni notification callback\n");
}

static void download_xtra_request_callback (void)
{
  fprintf(stdout, "*** xtra download request to client\n");
}

GpsCallbacks callbacks = {
  sizeof(GpsCallbacks),
  location_callback,
  status_callback,
  sv_status_callback,
  nmea_callback,
  set_capabilities_callback,
  acquire_wakelock_callback,
  release_wakelock_callback,
  create_thread_callback,
};

AGpsCallbacks callbacks2 = {
  agps_handle_status_callback,
  create_thread_callback,
};

AGpsRilCallbacks callbacks3 = {
  agps_ril_set_id_callback,
  agps_ril_refloc_callback,
  create_thread_callback,
};

GpsNiCallbacks callbacks4 = {
  ni_notify_callback,
  create_thread_callback,
};

GpsXtraCallbacks callbacks5 = {
  download_xtra_request_callback,
  create_thread_callback,
};

void sigint_handler(int signum)
{
  fprintf(stdout, "*** cleanup\n");
  if (Gps)
  {
	Gps->stop();
	Gps->cleanup();
  }
  exit (0);
}

int main(int argc, char *argv[])
{
  int sleeptime = 6000, opt, initok = 0;
  int coldstart = 0, extra = 0;
  struct timeval tv;
  int agps = 0, agpsril = 0, injecttime = 0;

  while ((opt = getopt(argc, argv, "acrtx")) != -1)
  {
               switch (opt) {
               case 'a':
		   agps = 1;
		   fprintf(stdout, "*** Using agps\n");
                   break;
	       case 'c':
		   coldstart = 1;
		   fprintf(stdout, "*** Using cold start\n");
		   break;
               case 'r':
		   agpsril = 1;
		   fprintf(stdout, "*** Using agpsril\n");
                   break;
	       case 't':
		   injecttime = 1;
		   fprintf(stdout, "*** Timing info will be injected\n");
                   break;
               case 'x':
                   extra = 1;
                   fprintf(stdout, "*** Allowing for Xtra downloads\n");
                   break;
               default:
                   fprintf(stderr, "\n Usage: %s \n \
			   \t-a for agps,\n \
			   \t-c for coldstarting the gps,\n \
		           \t-r for agpsril,\n \
			   \t-t to inject time,\n \
                           \t-x deal with Xtra gps data.\n \
			   \tnone for standalone gps\n",
                           argv[0]);
                   exit(1);
               }
  }


  fprintf(stdout, "*** setup signal handler\n");
  signal(SIGINT, sigint_handler);

  fprintf(stdout, "*** get gps interface\n");
  Gps = get_gps_interface();

  fprintf(stdout, "*** init gps interface\n");
  initok = Gps->init(&callbacks);
  fprintf(stdout, "*** setting positioning mode\n");
  /* need to be done before starting gps or no info will come out */
  if((agps||agpsril) && !initok)
	Gps->set_position_mode(GPS_POSITION_MODE_MS_BASED, GPS_POSITION_RECURRENCE_PERIODIC, 100, 5, 1000);
  else
	Gps->set_position_mode(GPS_POSITION_MODE_STANDALONE, GPS_POSITION_RECURRENCE_PERIODIC, 100, 5, 1000);

  if (Gps && !initok && (agps||agpsril))
  {
    fprintf(stdout, "*** get agps interface\n");
    AGps = get_agps_interface(Gps);
    if (AGps)
    {
	fprintf(stdout, "*** set up agps interface\n");
	AGps->init(&callbacks2);
	fprintf(stdout, "*** set up agps server\n");
	AGps->set_server(AGPS_TYPE_SUPL, "supl.google.com", 7276);
	fprintf(stdout, "*** Trying to open connection\n");
	AGps->data_conn_open("internet");
    }

    fprintf(stdout, "*** get agps ril interface\n");

    AGpsRil = get_agps_ril_interface(Gps);
    if (AGpsRil)
    {
	AGpsRil->init(&callbacks3);
    }

    /* if coldstart is requested, delete all location info */
    if(coldstart)
    {
      fprintf(stdout, "*** delete aiding data\n");
      Gps->delete_aiding_data(GPS_DELETE_ALL);
    }
    
    if(extra)
    {
      fprintf(stdout, "*** xtra aiding data init\n");
      GpsExtra = get_gps_extra_interface(Gps);
      if(GpsExtra)
        GpsExtra->init(&callbacks5);
    }

    fprintf(stdout, "*** setting up network notification handling\n");
    GpsNi = get_gps_ni_interface(Gps);
    if(GpsNi)
    {
      GpsNi->init(&callbacks4);
    }
  }

  if(injecttime)
  {
    fprintf(stdout, "*** aiding gps by injecting time information\n");
    gettimeofday(&tv, NULL);
    Gps->inject_time(tv.tv_sec, tv.tv_sec, 0);
  }

  fprintf(stdout, "*** start gps track\n");
  Gps->start();

  fprintf(stdout, "*** gps tracking started\n");

  while(sleeptime > 0)
  {
    fprintf(stdout, "*** tracking.... \n");
    sleep(100);
    sleeptime = sleeptime - 100;
  }

  fprintf(stdout, "*** stop tracking\n");
  Gps->stop();
  fprintf(stdout, "*** cleaning up\n");
  Gps->cleanup();

  return 0;
}
