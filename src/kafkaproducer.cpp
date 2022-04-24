#include "kafkaproducer.h"

#include <stdexcept>
#include <iostream>

namespace kafkaproducer
{
KafkaProducer::KafkaProducer(const std::string &endpoint, const std::string &topic,
  const kafkacb_t &msgCb):
  mProducer{nullptr},
  mEndpoint{endpoint},
  mTopic{topic},
  mEventCb{msgCb},
  mEndPooling{false}
{
  RdKafka::Conf* config = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  if (nullptr == config) {
    throw std::invalid_argument(ERR_MSG_INITIALIZE_CONFIG);
  }
  std::string err;
  if (RdKafka::Conf::CONF_OK != config->set("metadata.broker.list", mEndpoint, err)) {
    throw std::invalid_argument(std::string(ERR_MSG_SET_ENDPOINT) + ": " + err);
  }
  if (RdKafka::Conf::CONF_OK != config->set("event_cb", &mEventCb, err)){
    throw std::invalid_argument(std::string(ERR_MSG_SET_CALLBACK) + ": " + err);
  }
  mProducer.reset(RdKafka::Producer::create(config, err));
  delete config;
  if (nullptr == mProducer.get()) {
    throw std::invalid_argument(std::string(ERR_MSG_INITIALIZE_PRODUCER) + ": " + err);
  }
  mThread = std::thread([this]() {
    while (!mEndPooling) {
      mProducer->poll(100);
    }
  });
  this->createTopic(mTopic, [](const std::uint8_t retCode, const std::string& errstr) {
    if (ERR_SUCCESS != retCode && ERR_TOPIC_ALREADY_EXISTS != retCode) {
      throw std::invalid_argument(errstr);
    }
  });
}

KafkaProducer::~KafkaProducer() {
  mEndPooling = true;
  mThread.join();
  mProducer.reset();
}

bool KafkaProducer::produce(const std::string &message) const {
  auto err = mProducer->produce(mTopic, RdKafka::Topic::PARTITION_UA,
    RdKafka::Producer::RK_MSG_COPY,
    reinterpret_cast<void*>(const_cast<char*>(message.c_str())), message.length(),
    nullptr, 0, 0, nullptr);
  return RdKafka::ERR_NO_ERROR == err;
}

void KafkaProducer::createTopic(const std::string &topicName, const topiccb_t &cb) const
{
  std::string errstr;
  RdKafka::Topic *topic =
      RdKafka::Topic::create(mProducer.get(), topicName, NULL, errstr);
  if (nullptr == topic) {
    if(RD_KAFKA_RESP_ERR_TOPIC_ALREADY_EXISTS ==
       rd_kafka_last_error()) {
         cb(ERR_TOPIC_ALREADY_EXISTS, errstr);
    } else {
      cb(ERR_CREATE_TOPIC, errstr);
    }
  }
  delete topic;
  cb(ERR_SUCCESS, errstr);
}

} // namespace kafkaproducer