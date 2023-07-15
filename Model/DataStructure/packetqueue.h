#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL.h>

/**
 * @brief The packetqueue struct 缓冲队列
 */
typedef struct PacketQueue
{
    AVPacketList *first_pkt; //队头的一个 packet, 注意类型不是 AVPacket
    AVPacketList *last_pkt; //队尾 packet
    int nb_packets; // paket 个数
    int size; //
    SDL_mutex *mutex; //
    SDL_cond *cond; // 条件变量
}PacketQueue;

void packet_queue_init(PacketQueue *queue);
int packet_queue_put(PacketQueue *queue, AVPacket *packet);
int packet_queue_get(PacketQueue *queue, AVPacket *pakcet, int block);

#ifdef __cplusplus
}
#endif

#endif // PACKETQUEUE_H
