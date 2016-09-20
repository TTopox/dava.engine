import sys
import os
import shutil
import subprocess
import build_utils
import build_config

def get_supported_targets_for_build_platform(platform):
	if platform == 'win32':
		return ['win32', 'win10', 'android']
	else:
		return ['macos', 'ios', 'android']

def get_dependencies_for_target(target):
	return []

def get_supported_build_platforms():
	return ['win32', 'darwin']

def build_for_target(target, working_directory_path, root_project_path):
	if target == 'all':
		return __build_all_on_current_platform(working_directory_path, root_project_path)
	elif target == 'win32':
		return __build_win32(working_directory_path, root_project_path)
	elif target == 'win10':
		return __build_win10(working_directory_path, root_project_path)
	elif target == 'macos':
		return __build_macos(working_directory_path, root_project_path)
	elif target == 'ios':
		return __build_ios(working_directory_path, root_project_path)
	elif target == 'android':
		return __build_android(working_directory_path, root_project_path)

def get_download_url():
	return 'http://download.savannah.gnu.org/releases/freetype/freetype-2.7.tar.gz'

def __get_downloaded_archive_inner_dir():
	return 'freetype-2.7'

def __download_and_extract(working_directory_path):
	source_folder_path = os.path.join(working_directory_path, 'libfreetype_source')
	build_utils.download_and_extract(get_download_url(), working_directory_path, source_folder_path, __get_downloaded_archive_inner_dir())	
	return source_folder_path

def __patch_sources(source_folder_path, working_directory_path):
	try:
		if __patch_sources.did:
			return
	except AttributeError:
		pass

	build_utils.apply_patch(os.path.abspath('patch.diff'), working_directory_path)

	__patch_sources.did = True

def __build_win32(working_directory_path, root_project_path):
	source_folder_path = __download_and_extract(working_directory_path)
	__patch_sources(source_folder_path, working_directory_path)

	build_utils.build_and_copy_libraries_win32_cmake(
		os.path.join(working_directory_path, 'gen'), source_folder_path, root_project_path,
		'freetype.sln', 'freetype',
		'freetyped.lib', 'freetype.lib',
		'freetype246MT_D.lib', 'freetype246MT.lib',
		'freetype.lib', 'freetype.lib')

	return True

def __build_win10(working_directory_path, root_project_path):
	source_folder_path = __download_and_extract(working_directory_path)
	__patch_sources(source_folder_path, working_directory_path)

	build_utils.build_and_copy_libraries_win10_cmake(
		os.path.join(working_directory_path, 'gen'), source_folder_path, root_project_path,
		'freetype.sln', 'freetype',
		'freetyped.lib', 'freetype.lib',
		'freetype.lib', 'freetype.lib',
		'freetype.lib', 'freetype.lib',
		'freetype.lib', 'freetype.lib')

	return True

def __build_macos(working_directory_path, root_project_path):
	source_folder_path = __download_and_extract(working_directory_path)
	__patch_sources(source_folder_path, working_directory_path)

	build_utils.build_and_copy_libraries_macos_cmake(
		os.path.join(working_directory_path, 'gen'), source_folder_path, root_project_path,
		'freetype.xcodeproj', 'freetype',
		'libfreetype.a',
		'libfreetype_macos.a')

	return True

def __build_ios(working_directory_path, root_project_path):
	source_folder_path = __download_and_extract(working_directory_path)
	__patch_sources(source_folder_path, working_directory_path)

	build_utils.build_and_copy_libraries_ios_cmake(
		os.path.join(working_directory_path, 'gen'), source_folder_path, root_project_path,
		'freetype.xcodeproj', 'freetype',
		'libfreetype.a',
		'libfreetype_ios.a')

	return True

def __build_android(working_directory_path, root_project_path):
	source_folder_path = __download_and_extract(working_directory_path)
	__patch_sources(source_folder_path, working_directory_path)

	build_utils.build_and_copy_libraries_android_cmake(
		os.path.join(working_directory_path, 'gen'), source_folder_path, root_project_path,
		'libfreetype.a',
		'libfreetype.a')

	return True