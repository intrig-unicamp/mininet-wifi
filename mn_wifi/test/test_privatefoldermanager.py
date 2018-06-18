import unittest

from mininet.node import Node
from mn_wifi.utils.private_folder_manager import PrivateFolderManager


class MockNode(Node):
    def __init__(self):
        self.cmds = []

    def cmd(self, *args, **kwargs):
        self.cmds.append(args)


class TestPrivateFolderManager(unittest.TestCase):
    def test_mount(self):
        # todo improve test
        node = MockNode()
        folder_manager = PrivateFolderManager(node, [])
        assert len(node.cmds) is 0
        folder_manager.mount("/")
        assert len(node.cmds) is 2  # two for mount the dir
        try:
            folder_manager.mount("/", "/test")
            assert False  # we should never be where
        except ValueError:
            pass
        folder_manager.unmount("/")
        assert len(node.cmds) is 3  # the two from before plus one more
