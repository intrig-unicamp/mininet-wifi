import base64
from bitstring import BitArray


class adsbTransmitter(object):

    @classmethod
    def crc24(cls, msg_bits):
        poly = 0xFFF409
        reg = 0
        for i in range(len(msg_bits)):
            reg <<= 1
            bit = int(msg_bits[i])
            if ((reg >> 24) ^ bit) & 1:
                reg ^= poly
        return reg & 0xFFFFFF

    @classmethod
    def encode_fake(cls, lat, lon):
        lat_cpr = int((lat + 90) * 131072 / 180)
        lon_cpr = int((lon + 180) * 131072 / 360)
        return lat_cpr, lon_cpr

    @classmethod
    def build_adsb(cls, icao_hex="AABBCC", lat=-23.5505, lon=-46.6333, altitude=12000, odd_flag=0):
        df = BitArray(uint=17, length=5)
        ca = BitArray(uint=0, length=3)
        icao = BitArray(hex=icao_hex)
        type_code = BitArray(uint=11, length=5)
        n = int((altitude + 1000) / 25)
        alt_bits = BitArray(uint=n, length=11) + BitArray(uint=1, length=1)
        f_bit = BitArray(uint=odd_flag, length=1)
        lat_cpr, lon_cpr = cls.encode_fake(lat, lon)
        lat_bits = BitArray(uint=lat_cpr, length=17)
        lon_bits = BitArray(uint=lon_cpr, length=17)
        me = type_code + BitArray(uint=0, length=6) + alt_bits + f_bit + lat_bits + lon_bits
        msg = df + ca + icao + me
        crc = BitArray(uint=cls.crc24(msg.bin), length=24)
        return (msg + crc).tobytes()  # return bytes

    @classmethod
    def send_adsb_scapy(cls, airCraft, flight):
        try:
            msg = adsbTransmitter.build_adsb(airCraft.icao_24bit,
                                             flight.latitude,
                                             flight.longitude,
                                             flight.altitude,
                                             odd_flag=0)
            encoded = base64.b64encode(msg).decode('ascii')
            airCraft.pexec('echo {} > /tmp/mn-wifi-adsb-{}.log'.format(encoded, airCraft.name), shell=True)
        except:
            pass