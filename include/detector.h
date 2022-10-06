#include <Poco/Dynamic/Var.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Timezone.h>
#include <Poco/URI.h>
#include <config.h>
#include <exceptions.h>

#include <algorithm>
#include <memory>
#include <unordered_map>

struct Entry {
    int in;
    int out;
    Poco::Timestamp timestamp;

    Entry(int in, int out, int64_t timestamp) : in(in), out(out), timestamp(timestamp * 1000) {}
};

class AnomalyDetector {
  public:
    AnomalyDetector() {
        Poco::Timespan time_out_time(CRITICAL_TIME * Poco::Timespan::SECONDS);
        time_out_time_ = time_out_time;

        bot_path_ = "https://api.telegram.org/bot" + TG_TOKEN + "/";
        Poco::URI bot_uri(bot_path_ + "sendMessage");
        std::string scheme(bot_uri.getScheme());
        std::string host(bot_uri.getHost());
        auto port(bot_uri.getPort());

        if (scheme == "http") {
            tg_session_ = std::make_unique<Poco::Net::HTTPClientSession>(host, port);
        } else if (scheme == "https") {
            tg_session_ = std::unique_ptr<Poco::Net::HTTPClientSession>(
                new Poco::Net::HTTPSClientSession(host, port));
        } else {
            throw RequestError("Unknown scheme");
        }

        Poco::URI server_uri(URI);
        server_session_ = std::make_unique<Poco::Net::HTTPClientSession>(server_uri.getHost(),
                                                                         server_uri.getPort());
    }

    std::vector<Entry> getEntriesInRange(const Poco::Timestamp &start, const Poco::Timestamp &end) {
        Poco::URI uri(URI);
        auto start_time = start.raw();
        auto end_time = end.raw();
        Poco::URI::QueryParameters params = {{"bus_id", BUS_ID},
                                             {"door_id", DOOR_ID},
                                             {"start", std::to_string(start_time)},
                                             {"finish", std::to_string(end_time)}};
        uri.setQueryParameters(params);
        Poco::Net::HTTPRequest request("GET", uri.toString());
        server_session_->sendRequest(request);

        Poco::Net::HTTPResponse response;
        std::istream &resopndeBody = server_session_->receiveResponse(response);

        Poco::JSON::Parser parser;

        Poco::Dynamic::Var reply = parser.parse(resopndeBody);

        auto data = reply.extract<Poco::JSON::Object::Ptr>()->getObject("data")->getArray("result");
        std::vector<Entry> result;
        for (Poco::JSON::Array::ConstIterator it = data->begin(); it != data->end(); ++it) {
            auto time = it->extract<Poco::JSON::Object::Ptr>()->get("timestamp").toString();
            auto in = it->extract<Poco::JSON::Object::Ptr>()->get("in").toString();
            auto out = it->extract<Poco::JSON::Object::Ptr>()->get("out").toString();
            Entry entry = Entry(std::stoi(in), std::stoi(out), std::stoll(time));
            result.push_back(entry);
        }

        return result;
    }

    void notify(const std::string &msg, const Poco::Timestamp &last_timestamp) {
        std::string last_datapoint_dt =
            Poco::DateTimeFormatter::format(last_timestamp, "%H:%M:%S %z", Poco::Timezone::tzd());

        std::unordered_map<std::string, std::string> params;
        params["chat_id"] = CHAT_ID;
        params["text"] = msg + last_datapoint_dt;
        Poco::URI::QueryParameters query_params;
        for (const auto &el : params) {
            query_params.emplace_back(el);
        }

        Poco::URI uri(bot_path_ + "sendMessage");
        uri.setQueryParameters(query_params);
        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, uri.toString());

        Poco::Net::HTTPResponse response;
        tg_session_->sendRequest(request);
        std::istream &rs = tg_session_->receiveResponse(response);
        auto status = response.getStatus();
        if (status != Poco::Net::HTTPResponse::HTTPStatus::HTTP_OK) {
            throw RequestError(std::to_string(int(status)) + " " + response.getReason());
        }
    }

    Poco::Timestamp getLastTimestamp(const std::vector<Entry> &data) {
        auto max_element_it =
            std::max_element(data.begin(), data.end(), [](const auto &lhs, const auto &rhs) {
                return lhs.timestamp < rhs.timestamp;
            });
        return max_element_it->timestamp;
    }

    void start() {
        auto results = getEntriesInRange(Poco::Timestamp(START_TIMESTAMP), Poco::Timestamp());
        last_timestamp_ = getLastTimestamp(results);
        auto now = Poco::Timestamp();

        while (true) {
            if (Poco::Timespan(now - last_timestamp_) > time_out_time_) {
                notify(CRITICAL_TIME_EXCEEDED_MSG, last_timestamp_);
                do {
                    sleep(WAITING_TIME);
                    results = getEntriesInRange(last_timestamp_, Poco::Timestamp());
                } while (results.empty());

                last_timestamp_ = getLastTimestamp(results);
                notify(IT_WORKS_AGAIN_MSG, last_timestamp_);

            } else {
                auto to_sleep =
                    Poco::Timespan(CRITICAL_TIME, 0) - Poco::Timespan(now - last_timestamp_);
                sleep(to_sleep.seconds());
                results = getEntriesInRange(last_timestamp_, Poco::Timestamp());
                last_timestamp_ = getLastTimestamp(results);
            }
            now = Poco::Timestamp();
        }
    }

  private:
    Poco::Timestamp last_timestamp_;
    Poco::Timespan time_out_time_;
    std::unique_ptr<Poco::Net::HTTPClientSession> tg_session_;
    std::unique_ptr<Poco::Net::HTTPClientSession> server_session_;
    std::string bot_path_;
};
