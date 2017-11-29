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
            folder = private_folder[0] if isinstance(private_folder, tuple) else private_folder
            destination = private_folder[1] if isinstance(private_folder, tuple) else None
            self._mount(folder, destination)

    def mount(self, directory, destination=None):
        """
        Mounts a folder in another point


        :param directory: the directory path that will be virtually isolated
        :type directory: str
        :param destination: the destined directory to point the virtual directory
        :type destination: str
        """
        if directory in self.private_folders:
            raise ValueError("Folder already present, please unmount and mount again")
        # let's try to mount it
        self._mount(directory, destination)
        self.private_folders.append(directory)

    def _mount(self, private_dir, destination):
        """
        Makes the mount of the directories

        :type destination: str
        :type private_dir: str
        :param private_dir:
        :param destination:
        """
        if destination:
            # mount given private directory
            self.node.cmd('mkdir -p %s' % private_dir)
            self.node.cmd('mkdir -p %s' % destination)
            self.node.cmd('mount --bind %s %s' % (private_dir, destination))
        else:
            # mount temporary filesystem on directory
            self.node.cmd('mkdir -p %s' % private_dir)
            self.node.cmd('mount -n -t tmpfs tmpfs %s' % private_dir)

    def finish(self):
        """
        Cleans the mounted directories
        """
        for private_folder in self.private_folders:
            self.unmount(private_folder)

    def unmount(self, private_folder):
        """
        Unmounts a folder

        :param private_folder: the path of the folder to unmount
        :type private_folder: str
        """
        if private_folder in self.private_folders:
            self.private_folders.remove(private_folder)
        else:
            raise ValueError("Directory mount not found, maybe you didn't mount it before?")

        self.node.cmd('umount ', private_folder)
