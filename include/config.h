#include <string>

const std::string URI = "http://194.113.237.29/db/get_log_data";
const std::string BUS_ID = "bus_1";
const std::string DOOR_ID = "door_1";

const std::string TG_TOKEN = "5632355593:AAGKK4XW7nqxn9j1XdTGP-DRIptTjM83PQo";
const std::string CHAT_ID = "215497907";

const int CRITICAL_TIME = 60 * 60;
const int WAITING_TIME = 5 * 60;
const int64_t START_TIMESTAMP = 1665022334943969;

const std::string CRITICAL_TIME_EXCEEDED_MSG =
    "It doesn't work for long period. Last data point was at: ";
const std::string IT_WORKS_AGAIN_MSG = "It works again! Last data point was at: ";