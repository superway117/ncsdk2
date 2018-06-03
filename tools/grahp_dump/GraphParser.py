
#!/usr/bin/python
# written by Eason
# Mail eason.pan@intel.com

import binascii
import struct
import sys
import hashlib
from Graph import GraphHeader, Stage

from colorama import init
from colorama import Fore, Back, Style

class GraphParser(object):

	def __init__(self, filename, force=False, startOffset=0,
			 onlyParseHeader=False):
		init()
		self.header = None
		self.segments = list()
		self.fileParsed = False
		self.startOffset = startOffset
		self.data = bytearray()
	 
		# read file and convert data to list
		f = open(filename, "rb")
		f.seek(self.startOffset, 0)
		self.data = bytearray(f.read())
		f.close()

		# parse graph file
		self.parseElf(self.data, onlyParseHeader=onlyParseHeader)

		

	# this function parses the ELF file
	# return values: None
	def parseElf(self, buffer_list, onlyParseHeader=False):

		# large enough to contain e_ident?
		if len(buffer_list) < 16:
			raise ValueError("Buffer is too small to contain an ELF header.")

		self.header = GraphHeader()
 
		unpackedHeader = struct.unpack("< 10I", buffer_list[34:74])

		(
				self.header.magic,
				self.header.filesize,
				self.header.blob_vmajor,
				self.header.blob_vminor,    # 32/64 bit!
				self.header.no_shaves,    # 32/64 bit!
				self.header.stage_section_offset,    # 32/64 bit!
				self.header.buffer_section_offset,
				self.header.reloc_section_offset,
				self.header.stage.size_of_input,
				self.header.stage.permutation_enabled
		) = unpackedHeader

		# stage section
		stage_section_start = self.header.stage_section_offset
		stage_section_end = stage_section_start + 3*4

		unpackedHeader = struct.unpack("< 3I", buffer_list[stage_section_start:stage_section_end])

		(
				self.header.stage.stage_count,
				self.header.stage.size_of_stage_section,
				self.header.stage.size_of_output,
				
				
		) = unpackedHeader
		print(self.header.stage.permutation_enabled)
		if self.header.stage.permutation_enabled:
			unpackedHeader = struct.unpack("< I", buffer_list[stage_section_end:stage_section_end+4])

			(
					self.header.stage.permutation_size,
			) = unpackedHeader
			print(self.header.stage.permutation_size)
			#unpackedHeader = struct.unpack("< I", buffer_list[stage_section_end+4:stage_section_end+4+self.header.stage.permutation_size])

			#(
			#		self.header.stage.permutation_size,
			#) = unpackedHeader
		# reloc section
		reloc_section_start = self.header.reloc_section_offset
		reloc_section_end = reloc_section_start + 5*4
		unpackedHeader = struct.unpack("< 5I", buffer_list[reloc_section_start:reloc_section_end])

		(
				self.header.reloc.reloc_size,
				self.header.reloc.blob_reloc_offset,
				self.header.reloc.blob_reloc_size,
				self.header.reloc.work_reloc_offset,
				self.header.reloc.work_reloc_size
				 
		) = unpackedHeader

		# buffer section
		buffer_section_start = self.header.buffer_section_offset
		#buffer_section_end = buffer_section_start + 4*4
		unpackedHeader = struct.unpack("< I", buffer_list[buffer_section_start:buffer_section_start+4])

		(
				self.header.buffer.buffer_sec_size,
			 
		) = unpackedHeader


		# stage list
		#'''

		next_stage = stage_section_end
		for curr_stage in range(self.header.stage.stage_count):

			stage_start = next_stage
			stage_end = stage_start + 5*4
			stage = Stage()
			print("%d %d" % (stage_start,stage_end))
			unpackedHeader = struct.unpack("< 5I", buffer_list[stage_start:stage_end])
			(
			    stage.nextStage,
			    stage.stageType,
			    stage.implementation_flag,
			    stage.preOpType,
			    stage.postOpType
			     
			) = unpackedHeader
			print("%d %d %d" % (stage.nextStage,stage.stageType,stage.implementation_flag))
			next_stage = stage.nextStage + stage_section_start
			self.header.stage.stages.append(stage)
   		#'''


	 

		#buffer_section_start = self.header.reloc_section_offset
		#buffer_section_end
		self.fileParsed  = True
 
	  




	def print_graph(self):

		# check if the file was completely parsed before
		if self.fileParsed is False:
			raise ValueError("Operation not possible. " \
				+ "File was not completely parsed before.")

		# output header
		print
		print "\033[1;31;40mGraph header:\033[0m"
		print "-----------------------------------------"
		print "\033[1;32;40m%-8s%-10s%-8s%-8s%-20s%-20s%-20s%-20s\033[0m" % ("Magic", \
			"FileSize","VMajor","VMinor", "no_shaves", \
			"StageSectionOffset","BufferSectionOffset","RelocSectionOffset")
		print "0x%-6x%-10d%-8d%-8d%-20d%-20d%-20d%-20d" % (self.header.magic,
			self.header.filesize,
			self.header.blob_vmajor,
			self.header.blob_vminor,
			self.header.no_shaves,
			self.header.stage_section_offset,
			self.header.buffer_section_offset,
			self.header.reloc_section_offset
			)
		print
		print "\033[1;31;40mStage Section Header:\033[0m"
		print "-----------------------------------------"
		print "\033[1;32;40m%-18s%-18s%-18s%-18s%-18s\033[0m" % ("StageCount","StageSectionSize","InputSize","OuputSize","Permutation")
		print "%-18d%-18d%-18d%-18d%-18d" % (
			self.header.stage.stage_count,
			self.header.stage.size_of_stage_section,
			self.header.stage.size_of_input,
			self.header.stage.size_of_output,
			self.header.stage.permutation_enabled
			)
		print
		print "\033[1;31;40mReloc Section Header:\033[0m"
		print "-----------------------------------------"
		print "\033[1;32;40m%-18s%-18s%-18s%-18s%-18s\033[0m" % ("RelocSize","BlobRelocOffset","BlobRelocSize","WorkRelocOffset","WorkRelocSize")
		print "%-18d%-18d%-18d%-18d%-18d" % (
			self.header.reloc.reloc_size,
			self.header.reloc.blob_reloc_offset,
			self.header.reloc.blob_reloc_size,
			self.header.reloc.work_reloc_offset,
			self.header.reloc.work_reloc_size
			)

		print
		print "\033[1;31;40mBuffer Section Header:\033[0m"
		print "-----------------------------------------"
		print "\033[1;32;40m%-18s\033[0m" % ("BufferSize",)
		print "%-18d" % (
			self.header.buffer.buffer_sec_size
			)

		print
		print "\033[1;31;40mStages:\033[0m"
		print "-----------------------------------------"
		print "\033[1;32;40m     %-40s%18s\033[0m" % ("StageType","Flag")
		for idx, stage in enumerate(self.header.stage.stages):
			if stage.isHw():
				print "\033[1;32;40m%-5d%-40s%18d\033[0m" % (idx,
					stage.name(),
					stage.implementation_flag
					)
			else:
				print "%-5d%-40s%18d" % (idx,
					stage.name(),
					stage.implementation_flag
					)

	
 