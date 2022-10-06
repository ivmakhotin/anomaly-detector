#include <memory>
#include <unordered_map>
#include <algorithm>

#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/URI.h>
#include <Poco/Dynamic/Var.h>
#include <Poco/Timezone.h>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

#include <config.h>
#include <exceptions.h>

struct Entry {
    int in;
    int out;
    Poco::Timestamp timestamp;

    Entry (int in, int out, int64_t timestamp) : in(in), out(out), timestamp(timestamp*1000) {}
};

void notify(std::string msg, Poco::Timestamp last_timestamp) {
    std::string path = "https://api.telegram.org/bot" + TG_TOKEN + "/";
    Poco::URI uri(path + "sendMessage");
    std::string scheme(uri.getScheme());
    std::string host(uri.getHost());
    auto port(uri.getPort());
    std::unique_ptr<Poco::Net::HTTPClientSession> session;
    if (scheme == "http"){
        session = std::make_unique<Poco::Net::HTTPClientSession>(host, port);
    } else if (scheme == "https"){
        session = std::unique_ptr<Poco::Net::HTTPClientSession>(new Poco::Net::HTTPSClientSession(host, port));
    } else {
        throw RequestError("Unknown scheme");
    }

    std::string last_datapoint_dt = Poco::DateTimeFormatter::format(
   last_timestamp,
   "%H:%M:%S %z",
   Poco::Timezone::tzd()
   );

    std::unordered_map<std::string, std::string> params;
    params["chat_id"] = CHAT_ID;
    params["text"] = msg + last_datapoint_dt;
    Poco::URI::QueryParameters query_params;
    for (const auto& el : params){
        query_params.emplace_back(el);
    }

    uri.setQueryParameters(query_params);
    Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, uri.toString());

    Poco::Net::HTTPResponse response;
    session->sendRequest(request);
    std::istream& rs = session->receiveResponse(response);
    auto status = response.getStatus();
    std::cout << status << std::endl;
    if (status != Poco::Net::HTTPResponse::HTTPStatus::HTTP_OK){
        throw RequestError(std::to_string(int(status)) + " " + response.getReason());
    }

}

class AnomalyDetector {

public:
    AnomalyDetector() {
        
        Poco::Timespan timeOutTime(CRITICAL_TIME*Poco::Timespan::SECONDS);
        _timeOutTime = timeOutTime;

    }

    std::vector<Entry> getEntriesInRange(Poco::Timestamp start, Poco::Timestamp end) {
        Poco::URI uri(URI);
        auto start_time = start.raw();
        auto end_time = end.raw();
        std::cout << start_time << " " << end_time << std::endl;
        Poco::URI::QueryParameters params = { {"bus_id", BUS_ID},
                                          {"door_id", DOOR_ID},
                                          {"start",  std::to_string(start_time)},
                                          {"finish", std::to_string(end_time)}};
        uri.setQueryParameters(params);
        Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
        Poco::Net::HTTPRequest request("GET", uri.toString());
        session.sendRequest(request);

        Poco::Net::HTTPResponse response;
        std::istream& resopndeBody = session.receiveResponse(response);

        Poco::JSON::Parser parser;

        Poco::Dynamic::Var reply = parser.parse(resopndeBody);

        auto data = reply.extract<Poco::JSON::Object::Ptr>()->getObject("data")->getArray("result");
        std::vector<Entry> result;
        for (Poco::JSON::Array::ConstIterator it= data->begin(); it != data->end(); ++it) {
            auto time = it->extract<Poco::JSON::Object::Ptr>()->get("timestamp").toString();
            auto in = it->extract<Poco::JSON::Object::Ptr>()->get("in").toString();
            auto out = it->extract<Poco::JSON::Object::Ptr>()->get("out").toString();
            // std::cout << "time: " << time << "\t";
            // std::cout << "in: " << in << "\t";
            // std::cout << "out: " << out << std::endl;
            Entry entry = Entry(std::stoi(in), std::stoi(out), std::stoll(time));
            result.push_back(entry);
        }

        return result;
    }

    void start() {
        auto results = getEntriesInRange(Poco::Timestamp(START_TIMESTAMP), Poco::Timestamp());
        auto max_element_it = std::max_element(results.begin(), results.end(), [ ]( const auto& lhs, const auto& rhs )
                                                                            {
                                                                            return lhs.timestamp < rhs.timestamp;
                                                                            });
        _last_timestamp = max_element_it->timestamp;
        auto now = Poco::Timestamp();

        while (true) {
            if (Poco::Timespan(now - _last_timestamp) > _timeOutTime) {
                
                notify(CRITICAL_TIME_EXCEEDED_MSG, _last_timestamp);
                do {
                    sleep(WAITING_TIME);
                    results = getEntriesInRange(_last_timestamp, Poco::Timestamp());
                }
                while (results.empty());
                max_element_it = std::max_element(results.begin(), results.end(), [ ]( const auto& lhs, const auto& rhs )
                                                                            {
                                                                            return lhs.timestamp < rhs.timestamp;
                                                                            });
                _last_timestamp = max_element_it->timestamp;
                notify(IT_WORKS_AGAIN_MSG, _last_timestamp);
                now = Poco::Timestamp();
            } else {
                std::cout << "im here" << std::endl;
                auto toSleep = Poco::Timespan(CRITICAL_TIME, 0) - Poco::Timespan(now - _last_timestamp);
                sleep(toSleep.seconds());
                results = getEntriesInRange(_last_timestamp, Poco::Timestamp());
                max_element_it = std::max_element(results.begin(), results.end(), [ ]( const auto& lhs, const auto& rhs )
                                                                            {
                                                                            return lhs.timestamp < rhs.timestamp;
                                                                            });
                _last_timestamp = max_element_it->timestamp;
                now = Poco::Timestamp();
            }
        }
    }

private:
    Poco::Timestamp _last_timestamp;
    Poco::Timespan _timeOutTime;

};

int main() {
    
    //notify("Test", Poco::Timestamp());
    auto detector = AnomalyDetector();
//     auto results = detector.getEntriesInRange(Poco::Timestamp(START_TIMESTAMP), Poco::Timestamp());
   
//     for (auto it = results.begin(); it != results.end(); it++) {
//         std::cout << (it->timestamp).raw() << std::endl;
//     }

//     auto max_element_it = std::max_element(results.begin(), results.end(), [ ]( const auto& lhs, const auto& rhs )
// {
//    return lhs.timestamp < rhs.timestamp;
// });
//     std::cout << results.size() << std::endl;
//     std::cout << (max_element_it->timestamp).raw() << std::endl;   

    detector.start();
    return 0;
}