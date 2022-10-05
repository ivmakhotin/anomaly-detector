#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/URI.h>
#include <Poco/Dynamic/Var.h>

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

#include <config.h>


// struct Entry {
//     int in;
//     int out;
//     Timestamp timestamp;
// };

// Status notify(std::string msg);

// class AnomalyDetector {
//     AnomalyDetector() {

//     }

//     std::vector<Entry> getEntriesInRange(Timestamp start, Timestamp end);

    

// private:
//     Timestamp _last_timestamp;

// };

int main() {
    Poco::URI uri(URI);
    Poco::URI::QueryParameters params = { {"bus_id", BUS_ID},
                                          {"door_id", DOOR_ID},
                                          {"start", "1664810000"},
                                          {"finish","1664858381"}};
    uri.setQueryParameters(params);
    Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
    Poco::Net::HTTPRequest request("GET", uri.toString());
    session.sendRequest(request);

    Poco::Net::HTTPResponse response;
    std::istream& resopndeBody = session.receiveResponse(response);

    Poco::JSON::Parser parser;

    Poco::Dynamic::Var reply = parser.parse(resopndeBody);

    auto data = reply.extract<Poco::JSON::Object::Ptr>()->getObject("data")->getArray("result");

    for (Poco::JSON::Array::ConstIterator it= data->begin(); it != data->end(); ++it) {
        auto time = it->extract<Poco::JSON::Object::Ptr>()->get("timestamp").toString();
        auto in = it->extract<Poco::JSON::Object::Ptr>()->get("in").toString();
        auto out = it->extract<Poco::JSON::Object::Ptr>()->get("out").toString();
        std::cout << "time: " << time << "\t";
        std::cout << "in: " << in << "\t";
        std::cout << "out: " << out << std::endl;
    }

    return 0;
}