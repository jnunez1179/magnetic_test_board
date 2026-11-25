#include <inttypes.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <pb_decode.h>
#include <pb_encode.h>
#include "can_link.h"
#include "mtb.pb.h"
#include "link.pb.h"

#define SAMPLING_MSPS 3600000       /*3.6MSPS*/
#define AMOUNT_FREQUENCIES 15
#define MAX_SAMPLES 540

#define BROADCAST 1
#define OBC 0

/* Payload Metadata Template structure */
typedef struct  {
  MtbPayload_MsgType type;

  uint16_t test_id;
  uint8_t target_node;
  uint8_t priority;

  uint8_t record_count;

  bool broadcast;
} MtbData;

/*TODO: Move FrequencyResults to main.c to be stored/accessed from FLASH*/
/*Parameters used to formulate the different frequency results*/
typedef struct FrequencyResults  {
  uint32_t sampling_MSPS;
  uint32_t frequency_MHz[AMOUNT_FREQUENCIES];

  uint32_t time_start_us[AMOUNT_FREQUENCIES];
  uint32_t time_end_us[AMOUNT_FREQUENCIES];
  uint32_t time_shift_ns[AMOUNT_FREQUENCIES];

  uint16_t voltage_adc[AMOUNT_FREQUENCIES][MAX_SAMPLES];
  uint16_t voltage_count[AMOUNT_FREQUENCIES];
  uint16_t current_adc[AMOUNT_FREQUENCIES][MAX_SAMPLES];
  uint16_t current_count[AMOUNT_FREQUENCIES];
} FrequencyResults;

static void build_mtb_data(MtbData *mtb_variant, uint8_t type, uint8_t target_node,
    uint8_t priority, uint16_t *test_id, bool broadcast)
{
  mtb_variant->type = type;
  mtb_variant->target_node = target_node;
  mtb_variant->priority = priority;
  mtb_variant->test_id = test_id;
  mtb_variant->broadcast = broadcast;
  mtb_variant->record_count = AMOUNT_FREQUENCIES;
}

/*Fill each corresponding frequency range with metadata and measured data*/
static void build_frequency_records(MtbPayload *mtb_payload, const FrequencyResults *frequency_results)
{
  for (uint8_t i = 0; i < AMOUNT_FREQUENCIES; i++) {
    mtb_payload->records[i].frequency_MHz = frequency_results->frequency_MHz[i];
    mtb_payload->records[i].sampling_MSPS = frequency_results->sampling_MSPS;

    mtb_payload->records[i].time_start_us = frequency_results->time_start_us[i];
    mtb_payload->records[i].time_end_us   = frequency_results->time_end_us[i];
    mtb_payload->records[i].time_shift_ns = frequency_results->time_shift_ns[i];

    for (uint16_t j = 0; j < MAX_SAMPLES; j++) {
      mtb_payload->records[i].voltage_adc[j] = frequency_results->voltage_adc[i][j];
      mtb_payload->records[i].current_adc[j] = frequency_results->current_adc[i][j];
    }

    mtb_payload->records[i].voltage_adc_count = frequency_results->voltage_count[i];
    mtb_payload->records[i].current_adc_count = frequency_results->current_count[i];
  }
}

/*Encode Payload*/
static bool encode_mtb_payload(MtbPayload *mtb_payload, MtbData *mtb_data, uint32_t seq,
                        uint32_t *buffer, size_t *encoded_len)
{
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer, MtbPayload_size);
  mtb_payload->node_id = can_link_node_id();
  mtb_payload->test_id = mtb_data->test_id;
  mtb_payload->seq = seq;

  mtb_payload->type = mtb_data->type;

  mtb_payload->records_count = mtb_data->record_count;

  mtb_payload->target_node = mtb_data->target_node;
  mtb_payload->priority = mtb_data->priority;
  mtb_payload->uptime_ms = k_uptime_get_32();

  if (!pb_encode(&stream, MtbPayload_fields, &mtb_payload)) {
    LOG_ERR("Nanopb encode failed: %s", PB_GET_ERROR(&stream));
    return false;
  }

  *encoded_len = stream.bytes_written;
  return true;
}

static bool decode_message(const uint8_t *buffer, size_t len, LinkMessage *msg)
{
    pb_istream_t stream = pb_istream_from_buffer(buffer, len);

    *msg = (LinkMessage)LinkMessage_init_zero;
    if (!pb_decode(&stream, LinkMessage_fields, msg)) {
        LOG_ERR("Nanopb decode failed: %s", PB_GET_ERROR(&stream));
        return false;
    }

    return true;
}

static void on_can_message(const uint8_t *buffer, size_t len, uint8_t source_node,
               bool is_broadcast, void *user_data)
{
  LinkMessage msg;

  if (!decode_message(buffer, len, &msg)) {
      return;
  }

  LOG_INF("RX [Prio:%u] src:%u->dst:%u type:%u seq:%" PRIu32 " (BC:%d)",
      (unsigned int)msg.priority, source_node, (unsigned int)msg.target_node,
      (unsigned int)msg.type, msg.seq, is_broadcast);

  if (msg.type == LinkMessage_MsgType_START_TEST) {
    k_sem_give(&mtb_sem);
    return;
  }
}

    local_node = (uint8_t)can_link_node_id();

    LOG_INF("Zephyr ISO-TP duplex test node %u online. TX Period: %dms offset=%dms",
        (unsigned int)local_node, CONFIG_CAN_LINK_TX_PERIOD_MS,
        CONFIG_CAN_LINK_TX_START_OFFSET_MS);

    if (CONFIG_CAN_LINK_TX_START_OFFSET_MS > 0) {
        k_msleep(CONFIG_CAN_LINK_TX_START_OFFSET_MS);
    }

    build_mtb_data(mtb_data, MtbPayload_MsgType_DATA, CONFIG_CAN_LINK_PEER_NODE, 0U, &test_id, OBC)





  int ret = can_link_init(on_can_message, NULL);
  if (ret != 0) {
    LOG_ERR("CAN init failed: %d", ret);
    return 0;
  }











  return 0;
}









