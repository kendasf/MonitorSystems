//
//  webservices.cpp
//  WebGuiPP
//
//  Webservices module implementation
//
//  Created by Niksa Orlic on 25/08/15.
//  Copyright (c) 2015 Geolux. All rights reserved.
//

#include "../gnode/fs.h"
#include "../gnode/timers.h"
#include "../gnode/json.h"
#include "../gnode/path.h"
#include "../gnode/executor.h"
#include "webservices.h"
#include "std_fix.h"
#include "main.h"
#include "../CryptLibs/md5.h"
#include "FileRoutines.h"
#include "CommProt.h"
#include "FileRoutines.h"
#include "radar.h"
#include "cameraFuncts.h"
#include <sys/types.h>
#include <dirent.h>

#include "SystemUtilities.h"
#include "cameraFuncts.h"
#include "FileRoutines.h"

#include <deque>
#include <thread>
#include <mutex>

extern float SupplyVoltageLevel;

const char* key = "GeoluxDereSve";
const char* footer = "GeoluxFMCWFooter";

extern int CurrentlyDisplayedSpeed;

extern int TestModeBitmap;
extern int TestModeDuration;

extern int fileDirty;

extern volatile int PWMDutyCycle;

std::mutex g_snapshot_mutex;
std::string g_snapshot_pict;
bool g_take_snapshot = false;

void EncodeBase64(char *pData, unsigned int dataLength, char *pEncodedData);
void StringToHex(char *pHexString, unsigned char *pValueBigNum, int bigNumLength);
void VMSDriver_GetDimensions(int panelsConfig, int& width, int& height);

webservices::webservices() {
  std::srand(std::time(NULL));

  add_web_api("/css/theme", &webservices::css_theme);
  add_web_api("/get_salt", &webservices::get_salt);
  add_web_api("/get_auth_token", &webservices::get_auth_token);
  add_web_api("/test_token", &webservices::test_token);
  add_web_api("/get_status", &webservices::get_status);
  add_web_api("/change_param", &webservices::change_param);
  add_web_api("/list_folder", &webservices::list_folder);
  add_web_api("/delete_file", &webservices::delete_file);
  add_web_api("/get_bitmap", &webservices::get_bitmap);
  add_web_api("/get_bitmap_data", &webservices::get_bitmap_data);
  add_web_api("/enum_library_contents", &webservices::enum_library_contents);
  add_web_api("/get_library_bitmap", &webservices::get_library_bitmap);
  add_web_api("/get_library_bitmap_data", &webservices::get_library_bitmap_data);
  add_web_api("/save_bitmap", &webservices::save_bitmap);
  add_web_api("/get_dimming_table", &webservices::get_dimming_table);  
  add_web_api("/start_test_mode", &webservices::start_test_mode);
  add_web_api("/get_schedule", &webservices::get_schedule);
  add_web_api("/delete_schedule_entry", &webservices::delete_schedule_entry);
  add_web_api("/add_schedule", &webservices::add_schedule);
  add_web_api("/activate_schedule", &webservices::activate_schedule);
  add_web_api("/get_panels_config", &webservices::get_panels_config);
  add_web_api("/set_panels_config", &webservices::set_panels_config);
  add_web_api("/upload", &webservices::upload);
  add_web_api("/get_bvs_data", &webservices::get_bvs_data);
  add_web_api("/radar_autoconf", &webservices::radar_autoconf);

  add_web_api("/take_snapshot", &webservices::take_snapshot);
  add_web_api("/get_last_snapshot", &webservices::get_last_snapshot);

  new std::thread([]() {
    while(true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      if (g_take_snapshot) {
        std::string pict = takePicToMemory(75); 
        g_snapshot_mutex.lock(); 
        if (!pict.empty())
          std::swap(pict, g_snapshot_pict);
        g_snapshot_mutex.unlock();
      }
    }
  });

  gnode::set_interval([](){
    g_snapshot_mutex.lock();
    g_take_snapshot = false;
    g_snapshot_mutex.unlock();
  }, 30 * 1000);
}
              
void webservices::start(const std::string& folder, const std::string& rfolder, const std::string& upload_folder) {
  static_folder = folder;
  _root_folder = rfolder;
  _upload_folder = upload_folder;
  gnode::set_interval([this]() { uptime++; }, 1000);  
  gnode::set_interval(std::bind(&webservices::get_system_uptime, this), 30000);
  get_system_uptime();
}

webservices::response webservices::web_api(const std::string& func, const http::query& qs,
                                           const std::map<std::string, std::string, caseInsensitiveLT>& headers, const std::string& uploaded_path, const std::string& client_ip) {
  
  auto it = web_apis.find(func);
  if (it == web_apis.end())
    return response("Not found", 404);
  return (it->second)(qs, headers, uploaded_path, client_ip);
}

std::string webservices::ss_call(const std::string& func) {
  auto it = ss_calls.find(func);
  if (it == ss_calls.end())
    return "";
  return (it->second)();
}

template <class T>
void webservices::add_web_api(const std::string& n, T f) {
  web_apis.insert(std::make_pair(n, std::bind(f, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
}

template <class T>
void webservices::add_ss_call(const std::string& n, T f) {
  ss_calls.insert(std::make_pair(n, std::bind(f, this)));
}

void webservices::get_system_uptime() {
  std::string data = gnode::fs::read_file_sync("/proc/uptime");
  if(data.length() == 0)
    return;

  system_uptime = std::stoi(data); 
}

webservices::response webservices::get_salt(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  clear_expired_salts();

  srand(time(NULL));

  if (_salts.size() > 1000)
    return response("{}");

  std::string salt = "";
  std::string alphabet = "abcdefghijklmnopqrstuvxywzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-,;:_/?+*()!#$&<>[]{}";
  for (int i = 0; i < 16; i++) {
    salt += alphabet[rand() % alphabet.size()];
  }

  int salt_id = rand() % 65536;
  while (_salts.find(salt_id) != _salts.end()) {
    salt_id = rand() % 65536;
  }

  _salts[salt_id] = std::make_pair(salt, std::time(NULL));

  gnode::jsonizable salt_response;
  salt_response.property("salt", &salt);
  salt_response.property("id", &salt_id);
  return response(salt_response.to_string());
}

webservices::response webservices::radar_autoconf(const http::query& qs, const header_map& headers, const std::string& path, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  if (res != 0)
    return response("{\"status\": \"Not authenticated.\"}");

 if (Radar_AutoConf())
  return response("{\"status\": \"Radar configured successfully.\"}");

return response("{\"status\": \"Radar not responding.\"}");
}

webservices::response webservices::upload(const http::query& qs, const header_map& headers, const std::string& path, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  if (res != 0)
    return response("{\"status\": \"Not authenticated.\"}");

  if (!extract_uploaded_file_content(path, path + ".fl")) {
    gnode::fs::unlink_sync(path + ".fl");
    return response("{\"status\": \"Data corrupt.\"}");
  }

  FILEHANDLE fh = _sys_open((path + ".fl").c_str(), OPEN_R);
  int w, h;
  if (!FileRoutines_libraryCheckWidthHeight(fh, &w, &h)) {
    printf("Size: %d %d %s %d\n", w, h, path.c_str(), fh);
    _sys_close(fh);
    gnode::fs::unlink_sync(path + ".fl");

    DeviceInfoS DeviceInfo;
    int tw, th;
    FileRoutines_readDeviceInfo(&DeviceInfo);
    FileRoutines_DimensionsFromConfig(DeviceInfo.panelsConfiguration, &tw, &th);

    char response_str[128];
    sprintf(response_str, "{\"status\": \"Size of images in library does not match display size. Display size: %dx%d. Size of images in library: %dx%d.\"}", tw, th, w, h);
    return response(response_str);
  }

  _sys_close(fh);
  
  auto data = gnode::fs::read_file_sync(path + ".fl");
  
  gnode::fs::unlink_sync(path + ".fl");

  gnode::fs::write_file_sync("store/library.vil", data);

  return response("{\"status\": \"ok\"}");
}

webservices::response webservices::get_auth_token(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  clear_expired_salts();
  cleanup_auth_tokens();

  auto id_it = qs.params.find("id");
  auto level_it = qs.params.find("level");
  auto code_it = qs.params.find("code");

  if ((id_it == qs.params.end()) || (level_it == qs.params.end()) || (code_it == qs.params.end()))
    return response("{\"error_code\": 0}");

  auto salt_it = _salts.find(std::atoi(id_it->second.c_str()));
  if (salt_it == _salts.end())
    return response("{\"error_code\": 1}");

  auto salt = salt_it->second.first;
  _salts.erase(salt_it); 

  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  std::string unhashed_code = salt;
  if (std::atoi(level_it->second.c_str()) == 0)
    unhashed_code += deviceInfo.adminPassword;
  if (std::atoi(level_it->second.c_str()) == 1)
    unhashed_code += deviceInfo.password;
  if (std::atoi(level_it->second.c_str()) == 2)
    unhashed_code += deviceInfo.stupidPassword;

  auto hashed_code = md5(unhashed_code);

  if (hashed_code != code_it->second)
    return response("{\"error_code\": 2}");

  std::string token = "";
  std::string alphabet = "abcdefghijklmnopqrstuvxywzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for (int i = 0; i < 64; i++) {
    token += alphabet[rand() % alphabet.size()];
  }

  auto ua_it = headers.find("User-Agent");
  std::string ua = "";
  if (ua_it != headers.end())
    ua = ua_it->second;

  auth_token at = {
    std::time(NULL) + 60,
    token,
    client_ip,
    ua,
    std::atoi(level_it->second.c_str())
  };

  _auth_tokens.push_back(at);

  gnode::jsonizable token_response;
  token_response.property("token", &token);

  return response(token_response.to_string());
}

webservices::response webservices::test_token(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);
  return response("{\"code\" : " + std::to_string(res) + "}");
}

webservices::response webservices::get_status(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  if (res == -1)
    return response("{\"status\": \"invalid token\"}");

  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  std::string formatted_time;
  char tmpBuffer[64];
  sprintf(tmpBuffer, "%d", RTC_YEAR);
  formatted_time.append(tmpBuffer);
  formatted_time.append("-");
  sprintf(tmpBuffer, "%02d", RTC_MONTH);
  if (strlen(tmpBuffer) == 1)
    formatted_time.append("0");
  formatted_time.append(tmpBuffer);
  formatted_time.append("-");
  sprintf(tmpBuffer, "%02d", RTC_DOM);
  if (strlen(tmpBuffer) == 1)
    formatted_time.append("0");
  formatted_time.append(tmpBuffer);
  formatted_time.append(" ");
  sprintf(tmpBuffer, "%02d", RTC_HOUR);
  if (strlen(tmpBuffer) == 1)
    formatted_time.append("0");
  formatted_time.append(tmpBuffer);
  formatted_time.append(":");
  sprintf(tmpBuffer, "%02d", RTC_MIN);
  if (strlen(tmpBuffer) == 1)
    formatted_time.append("0");
  formatted_time.append(tmpBuffer);
  formatted_time.append(":");
  sprintf(tmpBuffer, "%02d", RTC_SEC);
  if (strlen(tmpBuffer) == 1)
    formatted_time.append("0");
  formatted_time.append(tmpBuffer);

  sprintf(tmpBuffer, "%s", FIRMWARE_VERSION);
  std::string formatted_firmware(tmpBuffer);

  //sprintf(tmpBuffer, "%d.%01d V", (int)SupplyVoltageLevel, ((int)(SupplyVoltageLevel * 10.00)) % 10);
  sprintf(tmpBuffer, "%0.01f V", SupplyVoltageLevel);
  std::string formatted_supply_voltage(tmpBuffer);

  sprintf(tmpBuffer, "%d lux", GetLuxmeterValue());
  std::string formatted_luminance(tmpBuffer);

  std::string formatted_speed_units = "mph";
  if (deviceInfo.unitType == 1)
    formatted_speed_units = "km/h";

  sprintf(tmpBuffer, "%d", deviceInfo.radarBaudRate);
  std::string formatted_radar_baud(tmpBuffer);

  std::string formatted_radar_protocol = "";
  switch(deviceInfo.radarProtocol) {
    case 0:
    formatted_radar_protocol  = "Spec. #28";
    break;
    case 1:
    formatted_radar_protocol  = "ASCII64";
    break;
    case 2:
    formatted_radar_protocol  = "No Radar";
    break;
    case 3:
    formatted_radar_protocol  = "NMEA";
    break;
    default:
    formatted_radar_protocol  = "Unknown";
    break;
  }

  sprintf(tmpBuffer, "%d", deviceInfo.sensitivity);
  std::string formatted_radar_sensitivity(tmpBuffer);

  sprintf(tmpBuffer, "%d.%d.%d.%d", 
    (deviceInfo.ipAddress >> 0) & 0x0ff,
    (deviceInfo.ipAddress >> 8) & 0x0ff,
    (deviceInfo.ipAddress >> 16) & 0x0ff,
    (deviceInfo.ipAddress >> 24) & 0x0ff
    );
  std::string formatted_ip_address(tmpBuffer);

  sprintf(tmpBuffer, "%d.%d.%d.%d", 
    (deviceInfo.subnetMask >> 0) & 0x0ff,
    (deviceInfo.subnetMask >> 8) & 0x0ff,
    (deviceInfo.subnetMask >> 16) & 0x0ff,
    (deviceInfo.subnetMask >> 24) & 0x0ff
    );
  std::string formatted_subnet_mask(tmpBuffer);

  sprintf(tmpBuffer, "%d.%d.%d.%d", 
    (deviceInfo.gatewayAddress >> 0) & 0x0ff,
    (deviceInfo.gatewayAddress >> 8) & 0x0ff,
    (deviceInfo.gatewayAddress >> 16) & 0x0ff,
    (deviceInfo.gatewayAddress >> 24) & 0x0ff
    );
  std::string formatted_gateway_address(tmpBuffer);

  std::string wifi_name(deviceInfo.deviceName);

  sprintf(tmpBuffer, "%d", deviceInfo.minDisplaySpeed);
  std::string min_speed(tmpBuffer);

  sprintf(tmpBuffer, "%d", deviceInfo.maxDisplaySpeed);
  std::string max_speed(tmpBuffer);

  sprintf(tmpBuffer, "%d", deviceInfo.blinkLimit);
  std::string speed_limit(tmpBuffer);

  sprintf(tmpBuffer, "%d", deviceInfo.blinkOnDurationMs);
  std::string blink_on(tmpBuffer);

  sprintf(tmpBuffer, "%d", deviceInfo.blinkOffDurationMs);
  std::string blink_off(tmpBuffer);

  sprintf(tmpBuffer, "%d", deviceInfo.displayBrightness);
  std::string display_brightness(tmpBuffer);

  sprintf(tmpBuffer, "%d", PWMDutyCycle);
  std::string actual_brightness(tmpBuffer);

  int dot_width, dot_height;
  VMSDriver_GetDimensions(deviceInfo.panelsConfiguration, dot_width, dot_height);

  gnode::jsonizable status_response;
  status_response.property("system-time", &formatted_time);
  status_response.property("firmware", &formatted_firmware);
  status_response.property("supply-voltage", &formatted_supply_voltage);
  status_response.property("luminance", &formatted_luminance);
  status_response.property("speed-units", &formatted_speed_units);
  status_response.property("radar-baud", &formatted_radar_baud);
  status_response.property("radar-protocol", &formatted_radar_protocol);
  status_response.property("radar-sensitivity", &formatted_radar_sensitivity);
  status_response.property("ip-address", &formatted_ip_address);
  status_response.property("subnet-mask", &formatted_subnet_mask);
  status_response.property("gateway-address", &formatted_gateway_address);
  status_response.property("wifi-name", &wifi_name);

  status_response.property("dot_width", &dot_width);
  status_response.property("dot_height", &dot_height);

  status_response.property("min-speed", &min_speed);
  status_response.property("max-speed", &max_speed);
  status_response.property("speed-limit", &speed_limit);

  status_response.property("blink-on", &blink_on);
  status_response.property("blink-off", &blink_off);
  status_response.property("display-brightness", &display_brightness);
  status_response.property("actual-brightness", &actual_brightness);

  return response(status_response.to_string());
}

webservices::response webservices::get_panels_config(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  if (res == -1)
    return response("{\"status\": \"invalid token\"}");

  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  int id = std::stod(qs["id"]);
  if ((id < 0) || (id > 3))
    return response("{\"status\": \"error\"}");

  int speedDisplayMode = deviceInfo.bitmapsConfig[id].speedDisplayMode;
  int x = deviceInfo.bitmapsConfig[id].x;
  int y = deviceInfo.bitmapsConfig[id].y;
  int font = deviceInfo.bitmapsConfig[id].font;
  int frameLength = deviceInfo.bitmapsConfig[id].frameLength;
  int numFrames = deviceInfo.bitmapsConfig[id].numFrames;

  std::deque<int> frames;
  for(int i = 0; i < 10; i++)
    frames.push_back(deviceInfo.bitmapsConfig[id].frames[i]);

  gnode::jsonizable status_response;
  status_response.property("speedDisplayMode", &speedDisplayMode);
  status_response.property("x", &x);
  status_response.property("y", &y);
  status_response.property("font", &font);
  status_response.property("frameLength", &frameLength);
  status_response.property("numFrames", &numFrames);
  status_response.property("frames", &frames);

  int dot_width, dot_height;
  VMSDriver_GetDimensions(deviceInfo.panelsConfiguration, dot_width, dot_height);

  status_response.property("imageWidth", &dot_width);
  status_response.property("imageHeight", &dot_height);

  return response(status_response.to_string());
}

webservices::response webservices::set_panels_config(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  if (res == -1)
    return response("{\"status\": \"invalid token\"}");

  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  int id = std::stod(qs["id"]);
  if ((id < 0) || (id > 3))
    return response("{}");

  deviceInfo.bitmapsConfig[id].speedDisplayMode = std::stoi(qs["speedDisplayMode"]);
  deviceInfo.bitmapsConfig[id].x = std::stoi(qs["x"]);
  deviceInfo.bitmapsConfig[id].y = std::stoi(qs["y"]);
  deviceInfo.bitmapsConfig[id].font = std::stoi(qs["font"]);
  deviceInfo.bitmapsConfig[id].frameLength = std::stoi(qs["frameLength"]);
  deviceInfo.bitmapsConfig[id].numFrames = std::stoi(qs["numFrames"]);

  for(int i = 0; i < 10; i++)
    deviceInfo.bitmapsConfig[id].frames[i] = std::stoi(qs["frame-" + std::to_string(i)]);

  FileRoutines_writeDeviceInfo(&deviceInfo);
  fileDirty = 1;

  return response("{}");
}

webservices::response webservices::get_schedule(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  if ((res != 0) && (res != 1))
    return response("{\"status\": \"invalid token\"}");

  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  int scheduleType = deviceInfo.scheduleType;

  std::string schedules = "";
  for (int i = 0; i < deviceInfo.schedulesCount; i++) {
    int entryType = deviceInfo.schedules[i].entryType;
    int dow = deviceInfo.schedules[i].dow;
    int year = deviceInfo.schedules[i].year;
    int month = deviceInfo.schedules[i].month;
    int day = deviceInfo.schedules[i].day;
    int start = deviceInfo.schedules[i].start;
    int duration = deviceInfo.schedules[i].duration;
    int periodStartDay = deviceInfo.schedules[i].periodStartDay;
    int periodStartMonth = deviceInfo.schedules[i].periodStartMonth;
    int periodEndDay = deviceInfo.schedules[i].periodEndDay;
    int periodEndMonth = deviceInfo.schedules[i].periodEndMonth;

    std::shared_ptr<gnode::jsonizable> entry = std::make_shared<gnode::jsonizable>();
    entry->property("entryType", &entryType);
    entry->property("dow", &dow);
    entry->property("year", &year);
    entry->property("month", &month);
    entry->property("day", &day);
    entry->property("start", &start);
    entry->property("duration", &duration);
    entry->property("sday", &periodStartDay);
    entry->property("smonth", &periodStartMonth);
    entry->property("eday", &periodEndDay);
    entry->property("emonth", &periodEndMonth);
    schedules += entry->to_string();
    if (i != (deviceInfo.schedulesCount - 1))
      schedules += ",";
  }

  return response("{\"schedule-type\": " + std::to_string(scheduleType) + ", \"schedules\": [" + schedules +   "]}");
}

webservices::response webservices::add_schedule(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  if ((res != 0) && (res != 1))
    return response("{\"status\": \"invalid token\"}");

  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  if (deviceInfo.schedulesCount >= MAX_SCHEDULES)
    return response("{\"status\" : \"Too many entries.\"}");

  deviceInfo.schedules[deviceInfo.schedulesCount].entryType = std::stoi(qs["entryType"]);
  deviceInfo.schedules[deviceInfo.schedulesCount].dow = std::stoi(qs["dow"]);
  deviceInfo.schedules[deviceInfo.schedulesCount].year = std::stoi(qs["year"]);
  deviceInfo.schedules[deviceInfo.schedulesCount].month = std::stoi(qs["month"]);
  deviceInfo.schedules[deviceInfo.schedulesCount].day = std::stoi(qs["day"]);
  deviceInfo.schedules[deviceInfo.schedulesCount].start = std::stoi(qs["start"]);
  deviceInfo.schedules[deviceInfo.schedulesCount].duration = std::stoi(qs["duration"]);

  deviceInfo.schedules[deviceInfo.schedulesCount].periodStartDay = std::stoi(qs["sday"]);
  deviceInfo.schedules[deviceInfo.schedulesCount].periodStartMonth = std::stoi(qs["smonth"]);
  deviceInfo.schedules[deviceInfo.schedulesCount].periodEndDay = std::stoi(qs["eday"]);
  deviceInfo.schedules[deviceInfo.schedulesCount].periodEndMonth = std::stoi(qs["emonth"]);

  deviceInfo.schedulesCount ++;

  FileRoutines_writeDeviceInfo(&deviceInfo);
  fileDirty = 1;

  return response("{}");
}

webservices::response webservices::activate_schedule(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  if ((res != 0) && (res != 1))
    return response("{\"status\": \"invalid token\"}");

  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  deviceInfo.scheduleType = std::stoi(qs["active"]);

  FileRoutines_writeDeviceInfo(&deviceInfo);
  fileDirty = 1;
  return response("{}");
}

webservices::response webservices::delete_schedule_entry(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  if ((res != 0) && (res != 1))
    return response("{\"status\": \"invalid token\"}");

  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  for(int i = std::stoi(qs["id"]); i < deviceInfo.schedulesCount - 1; i++) {
    deviceInfo.schedules[i] = deviceInfo.schedules[i + 1];
  }

  if (deviceInfo.schedulesCount > 0)
    deviceInfo.schedulesCount --;

  FileRoutines_writeDeviceInfo(&deviceInfo);
  fileDirty = 1;

  return response("{}");
}

webservices::response webservices::get_dimming_table(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  if (res != 0)
    return response("{\"status\": \"invalid token\"}");

  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  std::string json_response = "{\"table\": [";
  for (int i = 0; i < 16; i++) {
    json_response += "[" + std::to_string((int)deviceInfo.autoDimming[i].brightness) + "," + std::to_string(deviceInfo.autoDimming[i].luminance) + "]";

    if (i != 15)
      json_response += ",";
  }

  json_response += "]}";
  
  return response(json_response);
}

webservices::response webservices::start_test_mode(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  if (res == -1)
    return response("{\"status\": \"invalid token\"}");

  TestModeBitmap = std::stod(qs["image_id"]);
  TestModeDuration = std::stod(qs["duration"]) + 1;
  
  return response("{}");
}

webservices::response webservices::change_param(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  if (res == -1)
    return response("{\"status\": \"invalid token\"}");

  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  bool change_successfull = false;

  auto param_it = qs.params.find("param");
  if (param_it == qs.params.end()) {
    return response("{ \"error\" : \"1\"}");
  }
  auto value_it = qs.params.find("value");
  if (value_it == qs.params.end()) {
    return response("{ \"error\" : \"2\"}");
  }

  if (param_it->second == "radar-protocol") {
    if (res != 0)
      return response("{\"status\": \"invalid token\"}");

    if (value_it->second == "Spec.%20%2328") {
      deviceInfo.radarProtocol = (ProtocolTypeE)0;
      change_successfull = true;
    }
    if (value_it->second == "ASCII64") {
      deviceInfo.radarProtocol = (ProtocolTypeE)1;
      change_successfull = true;
    }
    if (value_it->second == "No%20Radar") {
      deviceInfo.radarProtocol = (ProtocolTypeE)2;
      change_successfull = true;
    }
    if (value_it->second == "NMEA") {
      deviceInfo.radarProtocol = (ProtocolTypeE)3;
      change_successfull = true;
    }

    if (change_successfull)
      Radar_Init((int)deviceInfo.radarBaudRate, (RadarProtocolE)deviceInfo.radarProtocol, deviceInfo.sensitivity);
  }

  if (param_it->second == "radar-baud") {
    if (res != 0)
      return response("{\"status\": \"invalid token\"}");

    if (value_it->second == "9600") {
      deviceInfo.radarBaudRate = 9600;
      change_successfull = true;
    }
    if (value_it->second == "38400") {
      deviceInfo.radarBaudRate = 38400;
      change_successfull = true;
    }
    if (value_it->second == "57600") {
      deviceInfo.radarBaudRate = 57600;
      change_successfull = true;
    }
    if (value_it->second == "115200") {
      deviceInfo.radarBaudRate = 115200;
      change_successfull = true;
    }

    if (change_successfull)
      Radar_Init((int)deviceInfo.radarBaudRate, (RadarProtocolE)deviceInfo.radarProtocol, deviceInfo.sensitivity);
  }

  if (param_it->second == "speed-units") {
    if (res != 0)
      return response("{\"status\": \"invalid token\"}");

    if (value_it->second == "mph") {
      deviceInfo.unitType = 0;
      change_successfull = true;
    }
    if (value_it->second == "km%2Fh") {
      deviceInfo.unitType = 1;
      change_successfull = true;
    }
  }

  if (param_it->second == "radar-sensitivity") {
    if (res != 0)
      return response("{\"status\": \"invalid token\"}");

    int val = std::stoi(value_it->second);
    if ((val >= 0) && (val <= 7)) {
      deviceInfo.sensitivity = val;
      Radar_Init((int)deviceInfo.radarBaudRate, (RadarProtocolE)deviceInfo.radarProtocol, deviceInfo.sensitivity);
      change_successfull = true;
    }
  }

  if (param_it->second == "pwd") {
    int level = std::stoi(qs["ul"]);
    std::string old_pwd_hash = qs["op"];
    std::string new_password = qs["value"];

    if ((level != res) && (res != 0)) {
      return response("{\"error\": \"Wrong privilege.\"}");
    }

    bool old_ok = false;
    std::string old_pwd = deviceInfo.adminPassword;
    if (md5(old_pwd) == old_pwd_hash)
      old_ok = true;
    if (!old_ok && (level == 1)) {
      old_pwd = deviceInfo.password;
      if (md5(old_pwd) == old_pwd_hash)
        old_ok = true;
    }
    if (!old_ok && (level == 2)) {
      old_pwd = deviceInfo.stupidPassword;
      if (md5(old_pwd) == old_pwd_hash)
        old_ok = true;
    }
    if (!old_ok)
      return response("{\"error\": \"The old password is not correct.\"}");

    if (new_password.length() / 2 > MAX_STRING_LENGTH)
      return response("{\"error\": \"The new password is too long.\"}");

    char new_password_raw[MAX_STRING_LENGTH];
    memset(new_password_raw, 0, MAX_STRING_LENGTH);
    StringToHex(const_cast<char*>(new_password.c_str()), (unsigned char*)new_password_raw, new_password.length() / 2);

    for (int i = 0; i < new_password.length() / 2; i++) {
      new_password_raw[i] = new_password_raw[i] ^ old_pwd[i % old_pwd.length()];
    }

    if (level == 0)
      strcpy(deviceInfo.adminPassword, new_password_raw);
    if (level == 1)
      strcpy(deviceInfo.password, new_password_raw);
    if (level == 2)
      strcpy(deviceInfo.stupidPassword, new_password_raw);
    change_successfull = true;
  }

  if (param_it->second == "panels-config") {
    if (res != 0)
      return response("{\"status\": \"invalid token\"}");

    int val = std::stoi(value_it->second);
    deviceInfo.panelsConfiguration = val;
    change_successfull = true;
  }

  if ((param_it->second == "system-time") && (value_it->second.size() >= 14)) {
    int year = std::stoi(value_it->second.substr(0, 4));
    int month = std::stoi(value_it->second.substr(4, 2));
    int day = std::stoi(value_it->second.substr(6, 2));
    int hour = std::stoi(value_it->second.substr(8, 2));
    int minute = std::stoi(value_it->second.substr(10, 2));
    int second = std::stoi(value_it->second.substr(12, 2));

    char params[128];
    sprintf(params, "hwclock --set -f /dev/rtc1 --date=\"%d-%02d-%02d %02d:%02d:%02d\"", 
      year, month, day, hour, minute, second);
    // add log
    FileRoutines_addLog(LOG_CHANGE_TIME, params);

    printf(">> set time: %s\n", params);

    //
    // set the time/dateof the RTC
    //
    auto resp = SysCmd(params); // Send command to set RTC
    if (resp != NULL) printf("Change time response: %s\r\n", resp);
    if (resp != NULL) free(resp);

    resp = SysCmd("hwclock -s -f /dev/rtc1"); // Updates OS from RTC
    if (resp != NULL) printf("Change time 2 response: %s\r\n", resp);
    if (resp != NULL) free(resp);

    setCameraTime((char*) CAMERA_IP_ADDR);

    update_auth_expiry(qs, headers, client_ip);

    change_successfull = true;
  }

  if (param_it->second == "wifi-name") {
    if ((res != 0) && (res != 1))
      return response("{\"status\": \"invalid token\"}");

    std::string p = qs["value"];
    if (p.length() >= MAX_STRING_LENGTH)
      p = p.substr(0, MAX_STRING_LENGTH - 1);
      
    strncpy(deviceInfo.deviceName, p.c_str(), p.size());
    deviceInfo.deviceName[p.size()] = 0;
    ChangeWifiName(deviceInfo.deviceName);
    change_successfull = true;
  }

  if ((param_it->second == "min-speed") && (qs["value"].length() > 0)) {
    if ((res != 0) && (res != 1))
      return response("{\"status\": \"invalid token\"}");

    deviceInfo.minDisplaySpeed = std::stoi(qs["value"]);
    CurrentlyDisplayedSpeed = -1;
    change_successfull = true;
  }

  if ((param_it->second == "max-speed") && (qs["value"].length() > 0)) {
    if ((res != 0) && (res != 1))
      return response("{\"status\": \"invalid token\"}");

    deviceInfo.maxDisplaySpeed = std::stoi(qs["value"]);
    deviceInfo.maxSpeedFromRotary = 0;
    CurrentlyDisplayedSpeed = -1;
    change_successfull = true;
  }

  if ((param_it->second == "speed-limit") && (qs["value"].length() > 0)) {
    deviceInfo.blinkLimit = std::stoi(qs["value"]);
    deviceInfo.blinkLimitFromRotary = 0;
    CurrentlyDisplayedSpeed = -1;
    change_successfull = true;
  }

  if ((param_it->second == "blink-on") && (qs["value"].length() > 0)) {
    if ((res != 0) && (res != 1))
      return response("{\"status\": \"invalid token\"}");

    deviceInfo.blinkOnDurationMs = std::stoi(qs["value"]);
    if (deviceInfo.blinkOnDurationMs > 5000)
      deviceInfo.blinkOnDurationMs = 5000;
    CurrentlyDisplayedSpeed = -1;
    change_successfull = true;
  }

  if ((param_it->second == "blink-off") && (qs["value"].length() > 0)) {
    if ((res != 0) && (res != 1))
      return response("{\"status\": \"invalid token\"}");

    deviceInfo.blinkOffDurationMs = std::stoi(qs["value"]);
    if (deviceInfo.blinkOffDurationMs > 5000)
      deviceInfo.blinkOffDurationMs = 5000;
    CurrentlyDisplayedSpeed = -1;
    change_successfull = true;
  }

  if ((param_it->second == "display-brightness") && (qs["value"].length() > 0)) {
    deviceInfo.displayBrightness = std::stoi(qs["value"]);
    if (deviceInfo.displayBrightness > 100)
      deviceInfo.displayBrightness = 100;
    if (deviceInfo.displayBrightness < 0)
      deviceInfo.displayBrightness = 0;
    DoAutoDimming(0);
    change_successfull = true;
  }

  if ((param_it->second == "autodimming") && (qs["value"].length() > 0)) {
    int autodimming = std::stoi(qs["value"]);
    if (autodimming)
      deviceInfo.displayBrightness |= 0x80;
    else
      deviceInfo.displayBrightness &= ~0x80;
    DoAutoDimming(0);
    change_successfull = true;
  }

  if (param_it->second == "dimming-table") {
    if (res != 0)
      return response("{\"status\": \"invalid token\"}");

    for (int i = 0; i < 16; i++) {
      deviceInfo.autoDimming[i].brightness = std::stod(qs["i" + std::to_string(i)]);
      deviceInfo.autoDimming[i].luminance = std::stod(qs["l" + std::to_string(i)]);
    }
    DoAutoDimming(0);
    change_successfull = true;
  }

  if ((param_it->second == "ip-address") && (qs["value"].length() > 0)) {
    if ((res != 0) && (res != 1))
      return response("{\"status\": \"invalid token\"}");

    deviceInfo.ipAddress = std::stoi(qs["value"]);
    SetupEthernetParams();
    change_successfull = true;
  }
  

  if (change_successfull) {
    FileRoutines_writeDeviceInfo(&deviceInfo);
    fileDirty = 1;
    return response("{ \"status\" : \"ok\"}");
  }

  return response("{ \"error\" : \"3\"}");
}

webservices::response webservices::list_folder(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  if (res == -1)
    return response("{\"status\": \"invalid token\", \"files\": []}");

  FINFO file_search_info;
  ffind("/dev/null/xyz", &file_search_info); // reset search params

  std::string file_search_mask = BASE_DATA;
  file_search_mask += qs["path"];

  if (file_search_mask[file_search_mask.size() - 1] == '/')
    file_search_mask = file_search_mask.substr(0, file_search_mask.size() - 1);

  if (file_search_mask.find("..") != std::string::npos)
    return response("{ \"status\": \"error\"}");

  std::string response_json = "{ \"status\": \"ok\", \"files\": [";

  char tmpBuffer[64];

  bool first = true;
  int ffindres;

  while((ffindres = ffind((char*)(file_search_mask.c_str()), &file_search_info)) != -2) {
    if (ffindres == -1)
      continue;
    if ((!file_search_info.valid) || (strlen(file_search_info.name) == 0))
      break;

    if (strcmp(file_search_info.name, "DeviceInfoS.dat") == 0)
      continue;

    if (first)
      first = false;
    else
      response_json += ",";

    if (file_search_info.attrib & ATTR_DIRECTORY) {
      response_json += "{\"name\" : \"";
      response_json += file_search_info.name;
      response_json += "\", \"dir\": \"true\", ";
    } else {
      response_json += "{\"name\" : \"";
      response_json += file_search_info.name;
      response_json += "\",";

      sprintf(tmpBuffer, "%d", file_search_info.size);
      response_json += "\"size\":";
      response_json += tmpBuffer;
      response_json += ",";      
    }

    sprintf(tmpBuffer, "%d-%02d-%02d %02d:%02d", 
      file_search_info.time.year, file_search_info.time.mon, file_search_info.time.day,
      file_search_info.time.hr, file_search_info.time.min);

    response_json += "\"date\": \"";
    response_json += tmpBuffer;
    response_json += "\"}";
  }

  response_json += "]}";

  return response(response_json);
}

webservices::response webservices::delete_file(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);
  if (res == -1)
      return response("{\"status\": \"invalid token\"}");
  
  std::string file_path = BASE_DATA;
  file_path += qs["path"];
  printf("Deleting: %s\n", file_path.c_str());
  gnode::fs::unlink_sync(file_path);

  return response("{}");
}

webservices::response webservices::get_bitmap(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  int id = std::stoi(qs["id"]);

  BitmapS bitmap;
  FileRoutines_readBitmap(id, &bitmap);

  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  int width = 60;
  int height = 48;
  VMSDriver_GetDimensions(deviceInfo.panelsConfiguration, width, height);

  std::string svg = "<svg width=\"" + std::to_string(width * 10) + "\" height=\"" + std::to_string(height * 10) + "\" viewBox=\"0 0 " + std::to_string(width * 10) + " " + std::to_string(height * 10) + "\"";
  svg += " xmlns=\"http://www.w3.org/2000/svg\">\n";
  svg += "<rect x=\"0\" y=\"0\" width=\"" + std::to_string(width * 10) + "\" height=\"" + std::to_string(height * 10) + "\"/>";

  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      int idx = 60 * j + i;
      if (bitmap.bitmapData[idx >> 3] & (1 << (idx & 0x7))) {
        svg += "<rect x=\"" + std::to_string(i * 10) + "\" y=\"" + std::to_string(j * 10) + "\" width=\"10\" height=\"10\" fill=\"#ffc200\" stroke=\"none\"/>\n";
      }
    }
  }

  svg += "</svg>";

  auto r = response(svg);  
  r.headers["Content-Type"] = "image/svg+xml";
  return r;
}

webservices::response webservices::get_bitmap_data(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);
  if (res == -1)
      return response("{\"status\": \"invalid token\"}");

  int id = std::stoi(qs["id"]);

  BitmapS bitmap;
  FileRoutines_readBitmap(id, &bitmap);
  
  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  int width = 60;
  int height = 48;

  VMSDriver_GetDimensions(deviceInfo.panelsConfiguration, width, height);

  std::string image_data = "{\"image_data\": [";

  for (int j = 0; j < height; j++) {
    image_data += "[";
    for (int i = 0; i < width; i++) {
      int idx = 60 * j + i;
      if (bitmap.bitmapData[idx >> 3] & (1 << (idx & 0x7)))
        image_data += "1";
      else
        image_data += "0";
      if (i != (width - 1))
        image_data += ",";
    }
    image_data += "]";
    if (j != (height - 1))
      image_data += ",";
  }

  image_data += "]}";

  return response(image_data);
}

inline int check_bitmap_bit(int w, int h, int x, int y, const char* hex_string) {
  if ((x < 0) || (y < 0) || (x >= w) || (y >= h))
    return 0;

  int idx = y * w + x;
  int digit_idx = idx >> 2;
  if ((digit_idx % 2) == 0)
    digit_idx ++;
  else
    digit_idx --;
  int subdigit_idx = idx & 0x03;

  int digit_val = 0;
  if ((hex_string[digit_idx] >= '0') && (hex_string[digit_idx] <= '9'))
    digit_val = hex_string[digit_idx] - '0';
  else if ((hex_string[digit_idx] >= 'A') && (hex_string[digit_idx] <= 'F'))
    digit_val = hex_string[digit_idx] - 'A' + 10;

  return digit_val & (1 << subdigit_idx);
}

webservices::response webservices::enum_library_contents(const http::query& qs, const header_map& headers, const std::string&, const std::string&) {
  static char enum_data[512];

  FileRoutines_libraryEnumImages(0, enum_data);
  int total_images = atoi(enum_data);

  FileRoutines_libraryEnumSequences(0, enum_data);
  int total_sequences = atoi(enum_data);

  std::string response_str = "{\"images\": [";

  int c = 0;
  bool first = true;
  while (c < total_images) {
    FileRoutines_libraryEnumImages(c, enum_data);
    char* tok = strtok(enum_data, ",");
    tok = strtok(NULL, ",\0");
    bool empty = true;
    while (tok) {
      if (!first)
        response_str.push_back(',');
      first = false;
      response_str.push_back('\"');
      response_str.append(tok);
      response_str.push_back('\"');
      c ++;
      empty = false;
      tok = strtok(NULL, ",\0");
    }

    if (empty)
      break;
  }

  response_str += "], ";
  response_str += "\"sequences\": [";

  c = 0;
  first = true;
  while (c < total_sequences) {
    FileRoutines_libraryEnumSequences(c, enum_data);
    char* tok = strtok(enum_data, ",");
    tok = strtok(NULL, ",\0");
    bool empty = true;
    while (tok) {
      if (!first)
        response_str.push_back(',');
      first = false;
      response_str.push_back('\"');
      response_str.append(tok);
      response_str.push_back('\"');
      c ++;
      empty = false;
      tok = strtok(NULL, ",\0");
    }

    if (empty)
      break;
  }

  response_str += "]}";

  return response(response_str);
}

webservices::response webservices::get_library_bitmap(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);

  int id = std::stoi(qs["id"]);

  char hex_image_data[60 * 48];
  int image_valid = FileRoutines_libraryReadImage(id, hex_image_data);

  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  int width = 60;
  int height = 48;
  VMSDriver_GetDimensions(deviceInfo.panelsConfiguration, width, height);

  std::string svg = "<svg width=\"" + std::to_string(width * 10) + "\" height=\"" + std::to_string(height * 10) + "\" viewBox=\"0 0 " + std::to_string(width * 10) + " " + std::to_string(height * 10) + "\"";
  svg += " xmlns=\"http://www.w3.org/2000/svg\">\n";
  svg += "<rect x=\"0\" y=\"0\" width=\"" + std::to_string(width * 10) + "\" height=\"" + std::to_string(height * 10) + "\"/>";

  if (image_valid) {
    for (int i = 0; i < width; i++) {
      for (int j = 0; j < height; j++) {
        if (check_bitmap_bit(width, height, i, j, hex_image_data)) {
          svg += "<rect x=\"" + std::to_string(i * 10) + "\" y=\"" + std::to_string(j * 10) + "\" width=\"10\" height=\"10\" fill=\"#ffc200\" stroke=\"none\"/>\n";
        }
      }
    }
  }

  svg += "</svg>";

  auto r = response(svg);  
  r.headers["Content-Type"] = "image/svg+xml";
  return r;
}

webservices::response webservices::get_library_bitmap_data(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);
  if (res == -1)
      return response("{\"status\": \"invalid token\"}");

  int id = std::stoi(qs["id"]);

  char hex_image_data[60 * 48];
  int image_valid = FileRoutines_libraryReadImage(id, hex_image_data);
  
  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  int width = 60;
  int height = 48;

  VMSDriver_GetDimensions(deviceInfo.panelsConfiguration, width, height);

  std::string image_data = "{\"image_data\": [";

  for (int j = 0; j < height; j++) {
    image_data += "[";
    for (int i = 0; i < width; i++) {
      if (image_valid && check_bitmap_bit(width, height, i, j, hex_image_data))
        image_data += "1";
      else
        image_data += "0";
      if (i != (width - 1))
        image_data += ",";
    }
    image_data += "]";
    if (j != (height - 1))
      image_data += ",";
  }

  image_data += "]}";

  return response(image_data);
}

webservices::response webservices::save_bitmap(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);
  
  if ((res != 0) && (res != 1))
      return response("{\"status\": \"invalid token\"}");

  int id = std::stoi(qs["id"]);
  auto data = qs["data"];

  if (data.length() != 2 * 60 * 48 / 8)
    return response("{ \"status\" : \"size incorrect\"}");

  BitmapS bitmap;
  StringToHex(const_cast<char*>(data.c_str()), (unsigned char*)bitmap.bitmapData, data.length() / 2);
  FileRoutines_writeBitmap(id, &bitmap);
  
  return response("{ \"status\" : \"ok\"}");
}

void webservices::clear_expired_salts() {
  for(auto it = _salts.begin(); it != _salts.end(); ) {
    if (std::time(NULL) > it->second.second + 60) {
      _salts.erase(it);
    }
    else
      ++ it;
  }
}

void webservices::cleanup_auth_tokens() {
  for(auto i = 0; i < _auth_tokens.size(); i++) {
    if (_auth_tokens[i].expiry < std::time(NULL)) {
      _auth_tokens.erase(_auth_tokens.begin() + i);
      --i;
    }
  }
}

int webservices::check_auth(const http::query& qs, const header_map& headers, const std::string& client_ip) {
  cleanup_auth_tokens();

  auto token_it = qs.params.find("token");
  if (token_it == qs.params.end()) {
    return -1;
  }

  auto ua_it = headers.find("User-Agent");
  std::string ua = "";
  if (ua_it != headers.end())
    ua = ua_it->second;

  for (int i = 0; i < _auth_tokens.size(); i++) {
    if (_auth_tokens[i].token == token_it->second) {
      if (_auth_tokens[i].client_ip != client_ip) {
        return -1;
      }

      if (_auth_tokens[i].user_agent != ua)
        return -1;

      _auth_tokens[i].expiry = std::time(NULL) + 180;

      return _auth_tokens[i].level;
    }
  }

  return -1;
}

int webservices::update_auth_expiry(const http::query& qs, const header_map& headers, const std::string& client_ip) {
  auto token_it = qs.params.find("token");
  if (token_it == qs.params.end()) {
    return 0;
  }

  auto ua_it = headers.find("User-Agent");
  std::string ua = "";
  if (ua_it != headers.end())
    ua = ua_it->second;

  for (int i = 0; i < _auth_tokens.size(); i++) {
    if (_auth_tokens[i].token == token_it->second) {
      if (_auth_tokens[i].client_ip != client_ip) {
        return 0;
      }

      if (_auth_tokens[i].user_agent != ua)
        return 0;

      _auth_tokens[i].expiry = std::time(NULL) + 180;

      return _auth_tokens[i].level;
    }
  }

  return 0;
}

webservices::response webservices::get_bvs_data(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto res = check_auth(qs, headers, client_ip);
  if (res == -1)
    return response("{\"status\": \"invalid token\"}");

  std::string file_path = BASE_DATA;
  file_path += qs["path"];

  FILEHANDLE fh = _sys_open((const char*)file_path.c_str(), OPEN_R);

  if (fh == -1)
  {
    return response("{\"status\": \"failed to open file\"}");
  }

  std::deque<int> values; 
  int entries = _sys_flen(fh) / 7;

  for (int i = 0; i < entries; i++) {
    unsigned char data[7];
    _sys_read(fh, data, 7, 0);

    for(int j = 0; j < 7; j++)
      values.push_back(data[j]);
  }

  _sys_close(fh);

  DeviceInfoS deviceInfo;
  FileRoutines_readDeviceInfo(&deviceInfo);

  gnode::jsonizable bvs_response;
  bvs_response.property("values", &values);
  bvs_response.property("speed_limit", &deviceInfo.blinkLimit);
  return response(bvs_response.to_string());
}

webservices::response webservices::make_redirect(const std::string& referer) {
  std::string res_str = "<html><head><meta http-equiv=\"Refresh\" content=\"0; url=";
  res_str += referer;
  res_str += "\"/></head></html>";
  response res(res_str, 303);
  res.headers.insert({"Location", referer});
  return res;
}

webservices::response webservices::css_theme(const http::query& qs, const webservices::header_map& headers, const std::string&, const std::string& client_ip) {
  std::string ui_theme = "cerulean"; 
  
  response res;
  res.data = gnode::fs::read_file_sync(gnode::path::join(static_folder, "css", "themes", "bootstrap." + ui_theme + ".min.css"));
  res.headers.insert({"Content-Type", "text/css"});
  return res;
}

webservices::response webservices::take_snapshot(const http::query& qs, const header_map& headers, const std::string&, const std::string&) {
  g_snapshot_mutex.lock();
  g_take_snapshot = true;
  g_snapshot_mutex.unlock();
  return response("{}");
}

webservices::response webservices::get_last_snapshot(const http::query& qs, const header_map& headers, const std::string&, const std::string& client_ip) {
  auto auth_res = check_auth(qs, headers, client_ip);
  if (auth_res == -1)
    return response("{\"status\": \"invalid token\"}");

  g_snapshot_mutex.lock();
  response res(g_snapshot_pict);
  g_snapshot_mutex.unlock();
  res.headers.insert({"Content-Type", "image/jpeg"});
  return res;
}

bool webservices::extract_uploaded_file_content(const std::string& infile, const std::string& outfile) {
  std::string boundary;
  std::deque<char> read_buffer;
  
  FILE *fin = fopen(infile.c_str(), "rb");
  if (fin == NULL)
    return false;
  
  FILE *fout = fopen(outfile.c_str(), "wb");
  if (fout == NULL) {
    fclose(fin);
    return false;
  }
  
  int c, prev_c = 0, nl_cnt = 0;
  enum { st_boundary, st_header, st_content } state;
  state = st_boundary;
  bool boundary_match = false;
  while (((c = fgetc(fin)) != EOF) && (!boundary_match)) {
    switch (state) {
      case st_boundary:
        if ((c != '\r') && (c != '\n'))
          boundary += (char)c;
        if (c == '\n')
          state = st_header;
        prev_c = c;
        break;
        
      case st_header:
        if (((c == '\r') && (prev_c == '\n')) || ((c == '\n') && (prev_c == '\r')))
          nl_cnt ++;
        else
          nl_cnt = 0;
        
        if (nl_cnt == 3)
          state = st_content;
        
        prev_c = c;
        break;
        
      case st_content:
        read_buffer.push_back(c);
        if (read_buffer.size() > boundary.size()) {
          fputc(read_buffer.front(), fout);
          read_buffer.pop_front();
          
          boundary_match = true;
          for(int i = 0; i < boundary.size(); i++) {
            if (boundary[i] != read_buffer[i]) {
              boundary_match = false;
              break;
            }
          }
        }
        break;
    }
  }
  
  fclose(fout);
  fclose(fin);
  
  return true;
}

