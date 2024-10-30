/*
** EPITECH PROJECT, 2024
** B-CPP-500-LYN-5-1-rtype-basile.fouquet (Workspace)
** File description:
** msg
*/

#pragma once

#include "net_common.hpp"
#include "net_huffman.hpp"
#include "structs/msg_all_structs.hpp"
#include "structs/msg_tcp_structs.hpp"
#include "structs/msg_udp_structs.hpp"
#include "utils/pack.hpp"

#include "msg_error.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace Network {

PACK(struct HeaderSerializedMessage {
    uint8_t type;
    uint64_t curSize;
});

PACK(struct UDPSerializedMessage {
    uint8_t type;
    uint64_t curSize;
    uint8_t flag;
    byte_t data[MAX_UDP_MSGLEN];
});

PACK(struct TCPSerializedMessage {
    uint8_t type;
    uint64_t curSize;
    bool isFinished = true;
    byte_t data[MAX_TCP_MSGLEN];
});

class AMessage {
public:
    enum Flag {
        COMPRESSED = 1,
        HEADER = 2,
        FRAGMENTED = 4,
        FULL_ACK = 8,
        ACK = 16,
        WAS_FRAGMENTED = 32,
        END_COMPRESS = 64,
    };

public:
    /* there is no constructor, the data is casted as the class it was meant to
     * be delivered */
    /* commented since we don't use AMessage anyway, so don't use the vtable */
    // virtual void writeHeader(); // override final;
    // virtual void readHeader(); // override final;

    uint64_t getSize() const {
        return m_curSize;
    }

    uint8_t getType() const {
        return m_type;
    }
    void setType(uint8_t type) {
        m_type = type;
    }

    /* THERE IS A DIFFERENCE BETWEEN isCompressed & isEndCompress */
    bool isCompressed(void) const {
        return m_flags & COMPRESSED;
    }
    bool isEndCompress(void) const {
        return m_flags & END_COMPRESS;
    }
    bool hasHeader(void) const {
        return m_flags & HEADER;
    }
    bool isFragmented(void) const {
        return m_flags & FRAGMENTED;
    }
    bool wasFragmented(void) const {
        return m_flags & WAS_FRAGMENTED;
    }
    bool isFullAck(void) const {
        return m_flags & FULL_ACK;
    }
    bool shouldAck(void) const {
        return m_flags & ACK;
    }

    uint8_t getFlags(void) const {
        return m_flags;
    }

    void setFlag(uint8_t flag) {
        m_flags = flag;
    }

    virtual const byte_t *getData(void) const = 0;
    virtual byte_t *getModifyData(void) = 0;
    virtual uint64_t getMaxMsgSize(void) const = 0;

    /******** WRITE DATA ********/

    template <typename T>
    size_t appendData(const T &data, size_t *sizeCompressWatcher = nullptr, size_t offset = 0) {
        if (m_compressNow) {
            if (m_curSize + offset > getMaxMsgSize())
                throw MsgError("Message offset is too big");
            m_huffman.compressContinuous(*this, m_curSize + offset, (const byte_t *)&data, sizeof(T));
            if (sizeCompressWatcher) {
                *sizeCompressWatcher += sizeof(T);
                if (*sizeCompressWatcher > getMaxMsgSize())
                    throw MsgError("Message is too big to compress");
            }
            return sizeof(T);
        }

        byte_t *myData = getDataMember();
        uint64_t maxSz = getMaxMsgSize();
        if (m_curSize + offset + sizeof(T) > maxSz)
            return sizeof(T);

        std::memcpy(myData + m_curSize + offset, &data, sizeof(T));
        m_curSize += sizeof(T);
        return sizeof(T);
    }

    template <typename T>
    void writeData(const T &data, size_t msgDataOffset = 0, size_t dataOffset = 0, bool updateSize = true) {
        if (m_compressNow) /* can't bceause the size will change */
            return;

        byte_t *myData = getDataMember();
        uint64_t maxSz = getMaxMsgSize();
        if (msgDataOffset + sizeof(T) >= maxSz)
            return;

        std::memcpy(myData + msgDataOffset, &data + dataOffset, sizeof(T));
        if (updateSize)
            m_curSize = sizeof(T);
    }

    template <typename T>
    size_t readData(T &data) const {
        if (m_compressNow)
            return -1; /* todo : raise the fact that you can't */

        if (sizeof(T) > m_curSize)
            throw MsgError("Message is too small to read data");

        const byte_t *myData = getData();
        std::memcpy(&data, myData, sizeof(T));
        return sizeof(T);
    }

    template <typename T>
    size_t readContinuousData(T &data, size_t &readOffset) const {
        if (sizeof(T) + readOffset > CF_NET_MIN(m_curSize, getMaxMsgSize()))
            throw MsgError("Message is too small to read data");

        const byte_t *myData = getData();
        std::memcpy(&data, myData + readOffset, sizeof(T));
        readOffset += sizeof(T);
        return sizeof(T);
    }

    template <typename T>
    size_t readContinuousCompressed(T &data, size_t &offset) {
        if (!isCompressed())
            throw MsgError("Message is not compressed");

        if (m_curSize + offset + getBitBuffer() / 8 > getMaxMsgSize())
            throw MsgError("Message overflow when reading");
        return m_huffman.decompressContinuous(*this, offset, (byte_t *)&data, sizeof(T));
    }

    size_t appendData(const void *data, std::size_t size, size_t *sizeCompressWatcher = nullptr);
    void writeData(const void *data, std::size_t size, bool updateSize = true);
    void readData(void *data, std::size_t &offset, std::size_t size) const;
    void readDataCompressed(void *data, std::size_t &offsetBits, std::size_t size);

    /****************************/

    /* When you call, all the appendeed data from here will be compressed, this is for END_COMPRESS */
    void startCompressingSegment(bool reading);
    /* When you call, everything that was in that previous segment will stop being compressed. THIS WILL APPLY THE SIZE
     * ONCE DONE */
    void stopCompressingSegment(bool reading);

    /*
        There is no:
            void startDecompressingSegment(void);
            void stopDecompressingSegment(void);

        since as soon we get the data, we decompress it, and append inside the message in the queue
    */

    bool getCompressingBuffer(void *&data, size_t &bufferSize);

    size_t &getBitBuffer(void) {
        return bitBuffer;
    }
    size_t &getBitRemains(void) {
        return bitBuffer;
    }

protected:
    AMessage(uint8_t type, uint8_t flags);
    virtual ~AMessage() = default;

    virtual byte_t *getDataMember() = 0;

    std::uint64_t m_curSize = 0;
    uint8_t m_type;
    uint8_t m_flags = 0;
    bool m_compressNow = false;

private:
    static Compression::AHC m_huffman;
    size_t bitBuffer = 0;
};

class TCPMessage : public AMessage {
public:
    TCPMessage(uint8_t type);
    ~TCPMessage() = default;

    TCPMessage &operator=(const TCPMessage &other);

    const byte_t *getData(void) const override final {
        return m_data;
    }
    byte_t *getModifyData(void) override final {
        return m_data;
    }
    uint64_t getMaxMsgSize(void) const override final {
        return MAX_TCP_MSGLEN;
    }

    void getSerialize(TCPSerializedMessage &msg) const;
    void setSerialize(const TCPSerializedMessage &msg);

private:
    byte_t *getDataMember() override final {
        return m_data;
    };

    bool m_isFinished = true;

    /* always set field to last, this is not a header !!!*/
    byte_t m_data[MAX_TCP_MSGLEN];
};

class UDPMessage : public AMessage {
public:
    /* hasHeader = tell if it will be transmitted into the channel, with the necessity to have additional headers */
    UDPMessage(uint8_t flags, uint8_t type);
    ~UDPMessage() = default;

    UDPMessage &operator=(const UDPMessage &other);

    const byte_t *getData(void) const override final {
        return m_data;
    }
    byte_t *getModifyData(void) override final {
        return m_data;
    }
    uint64_t getMaxMsgSize(void) const override final {
        return MAX_UDP_MSGLEN;
    }

    uint64_t getHash(void) const;

    void clear(void);

    void setCompressed(bool compressed);
    void setHeader(bool header);
    void setFragmented(bool fragmented);
    void setWasFragmented(bool fragmented);
    void setFullAck(bool fullack);
    void setAck(bool ack);

    void writeHeader(const UDPG_NetChannelHeader &header);
    void readHeader(UDPG_NetChannelHeader &header, size_t &readOffset) const;

    void getSerialize(UDPSerializedMessage &msg) const;
    std::vector<UDPSerializedMessage> getSerializeFragmented(void) const;
    void setSerialize(UDPSerializedMessage &msg);

    uint64_t getAckNumber(void) const;

private:
    byte_t *getDataMember() override final {
        return m_data;
    };

    /* always set field to last, this is not a header !!!*/
    byte_t m_data[MAX_UDP_MSGLEN];
};

} // namespace Network
