
#ifndef __KAFKA_PRODUCER__ 
#define __KAFKA_PRODUCER__

#include <librdkafka/rdkafka.h>
#include <librdkafka/rdkafkacpp.h>

#include <string>
#include <memory>
#include <cstdint>
#include <functional>
#include <thread>
#include <atomic>

namespace kafkaproducer {

constexpr auto ERR_MSG_INITIALIZE_CONFIG = "Unable to initialize config object";
constexpr auto ERR_MSG_SET_ENDPOINT = "Unable to set broker endpoint";
constexpr auto ERR_MSG_SET_CALLBACK = "Unable to set callback";
constexpr auto ERR_MSG_INITIALIZE_PRODUCER = "Unable to initialize kafka producer";

constexpr auto ERR_SUCCESS = 0;
constexpr auto ERR_TOPIC_ALREADY_EXISTS = 1;
constexpr auto ERR_CREATE_TOPIC = 2;

using kafkacb_t = std::function<void(RdKafka::Event &)>;

class KafkaProducer final {
 public:
  using topiccb_t = std::function<void(const std::uint8_t, const std::string &)>;
  KafkaProducer() = delete;
  explicit KafkaProducer(const std::string &, const std::string &, const kafkacb_t &);
  KafkaProducer(const KafkaProducer &) = default;
  KafkaProducer(KafkaProducer &&) = default;
  ~KafkaProducer();

  bool produce(const std::string &) const;

 private:
  void createTopic(const std::string &, const topiccb_t &) const;

  std::unique_ptr<RdKafka::Producer> mProducer;
  std::string mEndpoint;
  std::string mTopic;

  class EventCb : public RdKafka::EventCb {
   public:
    EventCb() = delete;
    EventCb(const kafkacb_t &kafkaCb) : mKafkaCb{kafkaCb} {}
    EventCb(const EventCb &) = default;
    EventCb(EventCb &&) = default;
    ~EventCb() = default;
    inline void event_cb(RdKafka::Event &event) { mKafkaCb(event); }
   private:
    kafkacb_t mKafkaCb;
  } mEventCb;

  std::thread mThread;
  std::atomic<bool> mEndPooling;
};

} // namespace kafkaproducer

#endif