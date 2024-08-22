# author: Ramon Fontes (ramon.fontes@ufrn.br)

from mn_wifi.btvirt.module import module


class Cleanup(object):
    "Wrapper for cleanup()"

    @classmethod
    def cleanup_btvirt(cls):
        """Clean up junk which might be left over from old runs;
           do fast stuff before slow dp and link removal!"""
        module.stop()


cleanup = Cleanup.cleanup_btvirt