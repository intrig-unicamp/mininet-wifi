from typing import List, Union, Tuple


class PrivateFolderManager(object):
    def __init__(self, node, private_folders):
        """
        Class to Manage private folders for a particular stations

        To add a new private folder use mount(...) with the folder or a tuple of virtual folder and destinated folder
        To remove a private folder use unmount(...) with the same tuple of just the

        :param node: the node where this private folders belong
        :type node: Node

        :param private_folders: the private folders, each can be a tuple with the path and destination folder,\
                                 or a single string with the folder that should be virtualized
        :type private_folders: List[Union[str, Tuple[str, str]]]
        """
        self.node = node  # type: Node
        self.private_folders = private_folders  # type: List[Union[str, Tuple[str, str]]]
        for private_folder in self.private_folders:
            self._mount(private_folder)

    def mount(self, directories):
        folder = directories[0] if isinstance(directories, tuple) else directories
        if folder in filter(lambda x: x[0] if isinstance(x, tuple) else x, self.private_folders):
            raise ValueError("Folder already present, please unmount and mount again")
        # let's try to mount it
        self._mount(directories)
        self.private_folders.append(directories)

    def _mount(self, directory):
        if isinstance(directory, tuple):
            # mount given private directory
            private_dir = directory[1] % self.__dict__
            mount_point = directory[0]
            self.node.cmd('mkdir -p %s' % private_dir)
            self.node.cmd('mkdir -p %s' % mount_point)
            self.node.cmd('mount --bind %s %s' % (private_dir, mount_point))
        else:
            # mount temporary filesystem on directory
            self.node.cmd('mkdir -p %s' % directory)
            self.node.cmd('mount -n -t tmpfs tmpfs %s' % directory)

    def finish(self):
        for private_folder in self.private_folders:
            self.unmount(private_folder)

    def unmount(self, private_folder):
        directory = private_folder[0] if isinstance(private_folder, tuple) else private_folder
        if private_folder in self.private_folders:
            self.private_folders.remove(private_folder)
        else:
            # the used added a private folder with a tuple of (origin, destination) but is removing with just the origin
            entry = filter(lambda x: x if (x[0] if isinstance(x, tuple) else x) == directory else None,
                           self.private_folders)
            if not entry:
                raise ValueError("Directory mount not found")

        self.node.cmd('umount ', directory)
