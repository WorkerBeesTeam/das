import platform
import os.path as p
import subprocess
import sys
from pathlib import Path

def get_pybind11_includes():
	try:
		result = subprocess.run(
			[sys.executable, "-m", "pybind11", "--includes"],
			check=True, capture_output=True, text=True
		)
		return result.stdout.strip().split(' ')
	except Exception as e:
		print(f"Ошибка при выполнении команды: {e}")
		return ['-I', '/usr/include/python3.13', '-I', '/usr/lib/python3/dist-packages/pybind11/include']

DIR_OF_THIS_SCRIPT = p.abspath( p.dirname( __file__ ) )
DIR_OF_THIRD_PARTY = p.join( DIR_OF_THIS_SCRIPT, 'dist/include' )
SOURCE_EXTENSIONS = [ '.cpp', '.cc', '.c', '.m', '.mm' ]

QT_INFO = subprocess.run(['qmake6', '-query'], stdout=subprocess.PIPE).stdout.decode().split("\n")
def getQtParam(param):
	return ''.join([i.split(param)[1] for i in QT_INFO if i.startswith(param)])
QT_HOSTDATA = getQtParam('QT_HOST_DATA:')
QT_HEADERS = getQtParam('QT_INSTALL_HEADERS:')
QT_SPEC = getQtParam('QMAKE_SPEC:')
QT_VERSION = getQtParam('QT_VERSION:')

flags = [
	'-Wall',
	'-Wextra',
	'-Werror',
	'-x',
	'c++',
	'-std=c++23',
	'-DDEBUG_BUILD=1',
	'-DHAS_PPROF=1',
	'-I', QT_HEADERS,
	'-I', QT_HEADERS + '/QtCore',
	'-I', QT_HEADERS + '/QtCore/' + QT_VERSION,
	'-I', QT_HEADERS + '/QtCore/' + QT_VERSION + '/QtCore',
	'-I', QT_HEADERS + '/QtGui',
	'-I', QT_HEADERS + '/QtGui/' + QT_VERSION,
	'-I', QT_HEADERS + '/QtGui/' + QT_VERSION + '/QtGui',
	'-I', QT_HEADERS + '/QtQuick/' + QT_VERSION,
	'-I', QT_HEADERS + '/QtQml',
	'-I', QT_HEADERS + '/QtNetwork',
	'-I', QT_HOSTDATA + '/mkspecs/' + QT_SPEC,
	'-I', DIR_OF_THIS_SCRIPT + '/dist/include',
	'-I', DIR_OF_THIS_SCRIPT + '/src',
	'-I', DIR_OF_THIS_SCRIPT + '/build_debug/src',
	'-I', DIR_OF_THIS_SCRIPT + '/build_debug/include',
	'-I', DIR_OF_THIS_SCRIPT + '/helpz/include',
	'-I', DIR_OF_THIS_SCRIPT + '/lib',
	'-I', DIR_OF_THIS_SCRIPT + '/build/Desktop-Debug/helpz/include',
	'-I', '/usr/include/glib-2.0',
	'-I', '/usr/lib/x86_64-linux-gnu/glib-2.0/include',
	*get_pybind11_includes()
]



# Set this to the absolute path to the folder (NOT the file!) containing the
# compile_commands.json file to use that instead of 'flags'. See here for
# more details: http://clang.llvm.org/docs/JSONCompilationDatabase.html
#
# You can get CMake to generate this file for you by adding:
#	set( CMAKE_EXPORT_COMPILE_COMMANDS 1 )
# to your CMakeLists.txt file.
#
# Most projects will NOT need to set this to anything; you can just change the
# 'flags' list of compilation flags. Notice that YCM itself uses that approach.
compilation_database_folder = ''
database = None


def IsHeaderFile( filename ):
	extension = p.splitext( filename )[ 1 ]
	return extension in [ '.h', '.hxx', '.hpp', '.hh' ]


def replaceIncludeWithWrc(path: str) -> str:
	"""Заменяет /include/*/ на /src/"""
	p = Path(path)
	parts = list(p.parts)

	if 'include' in parts:
		i = parts.index('include')
		# Заменяем include и следующий за ним компонент на 'src'
		parts[i:i+2] = ['src']
		return str(Path(*parts))
	return path


def FindCorrespondingSourceFile( filename ):
	if IsHeaderFile( filename ):
		basename = p.splitext( filename )[ 0 ]
		for extension in SOURCE_EXTENSIONS:
			replacementFile = basename + extension
			if p.exists( replacementFile ):
				return replacementFile

			replacementFile = replaceIncludeWithWrc(replacementFile)
			if p.exists( replacementFile ):
				return replacementFile
	return filename


def Settings( **kwargs ):
	# Do NOT import ycm_core at module scope.
	import ycm_core

	global database
	if database is None and p.exists( compilation_database_folder ):
		database = ycm_core.CompilationDatabase( compilation_database_folder )

	if kwargs[ 'language' ] == 'cfamily':
		# If the file is a header, try to find the corresponding source file and
		# retrieve its flags from the compilation database if using one. This is
		# necessary since compilation databases don't have entries for header files.
		# In addition, use this source file as the translation unit. This makes it
		# possible to jump from a declaration in the header file to its definition
		# in the corresponding source file.
		print("FILE_NAME=", kwargs[ 'filename' ])
		filename = FindCorrespondingSourceFile( kwargs[ 'filename' ] )

		if not database:
			finalFlags = flags
			if ('win' in filename):
				finalFlags = flags.copy()
				finalFlags += [
					'-I', '/usr/x86_64-w64-mingw32/include',
					'-D', '_WIN32', '-D', '_WIN64', '-D', 'WIN32', '-D', '_WINDOWS',
					'-D_MINGW32_', '-D__MINGW32__', '-D__MINGW64__',
					'-D__MINGW_VERSION=*',
					'-D__MINGW_MAJOR_VERSION=*',
					'-D__MINGW_IMPORT=',
					'-D_WIN32_WINNT=0x0601',
					'-DWINVER=0x0601',
					'-D__USE_MINGW_ANSI_STDIO=1',
					'-D_POSIX_C_SOURCE=200809L',
					'-D_REENTRANT']

			return {
				'flags': finalFlags,
				'include_paths_relative_to_dir': DIR_OF_THIS_SCRIPT,
				'override_filename': filename
			}

		compilation_info = database.GetCompilationInfoForFile( filename )
		if not compilation_info.compiler_flags_:
		  return {}

		# Bear in mind that compilation_info.compiler_flags_ does NOT return a
		# python list, but a "list-like" StringVec object.
		final_flags = list( compilation_info.compiler_flags_ )

		# NOTE: This is just for YouCompleteMe; it's highly likely that your project
		# does NOT need to remove the stdlib flag. DO NOT USE THIS IN YOUR
		# ycm_extra_conf IF YOU'RE NOT 100% SURE YOU NEED IT.
		try:
			final_flags.remove( '-stdlib=libc++' )
		except ValueError:
			pass

		return {
			'flags': final_flags,
			'include_paths_relative_to_dir': compilation_info.compiler_working_dir_,
			'override_filename': filename
		}
	elif kwargs[ 'language' ] == 'python':
		sysPaths = sys.path
		sysPaths.insert(0, str(p.join( DIR_OF_THIS_SCRIPT, 'build_debug/libipc/py' )))
		return {
			'interpreter_path': p.join( DIR_OF_THIS_SCRIPT, '.venv/bin/python' ),
			'sys_path': sysPaths
		}
	return {}

