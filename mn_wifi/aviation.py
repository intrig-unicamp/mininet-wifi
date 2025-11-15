import base64
import time
import json
from bitstring import BitArray


class aviationProtocol(object):

    @classmethod
    def load(cls, airCraft, flight, proto):
        if proto == 'adsb':
            adsbTransmitter.send_adsb_scapy(airCraft, flight)
        elif proto == 'adsc':
            adscTransmitter.send_adsc_scapy(airCraft, flight)


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
    def build_adsb(cls, airCraft, flight, odd_flag=0):

        icao_hex = airCraft.icao_24bit
        lat = flight.latitude
        lon = flight.longitude
        altitude = flight.altitude

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
            msg = adsbTransmitter.build_adsb(airCraft, flight, odd_flag=0)
            encoded = base64.b64encode(msg).decode('ascii')
            airCraft.pexec('echo {} > /tmp/mn-wifi-adsb-{}.log'.format(encoded, airCraft.name), shell=True)
        except:
            pass


class adscTransmitter(object):

    @classmethod
    def build_adsc(cls, airCraft, flight):
        icao_hex = airCraft.icao_24bit
        lat = flight.latitude
        lon = flight.longitude
        altitude = flight.altitude
        destination_airport_iata = flight.destination_airport_iata
        heading = flight.heading
        ground_speed = flight.ground_speed
        registration = flight.registration

        return {
            "msg_type": "ADS-C_REPORT",
            "icao": icao_hex,
            "flight_id": registration,
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "position": {"lat": lat, "lon": lon, "alt_ft": altitude},
            "velocity": {"gs_kts": ground_speed, "heading_deg": heading},
            "next_waypoint": destination_airport_iata
        }

    @classmethod
    def send_adsc_scapy(cls, airCraft, flight):
        try:
            msg = adscTransmitter.build_adsc(airCraft, flight)
            msg_json = json.dumps(msg)
            airCraft.pexec(f"echo '{msg_json}' > /tmp/mn-wifi-adsc-{airCraft.name}.log", shell=True)
        except:
            pass