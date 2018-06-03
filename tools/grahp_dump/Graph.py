#!/usr/bin/python
# written by Eason
# Mail eason.pan@intel.com

class Stage(object):
	STAGE_NAME_TABLE = {0: "kConv", 1: "kMaxPool", 2: "kAvgPool",
			3: "kSoftMax", 4: "kFC", 5: "kNone0", 6: "kRelu",
			7: "kReluX", 8: "kDepthConv", 9: "kBias",
			10: "kPRelu", 11: "kLRN", 12: "kSum",
			13: "kProd", 14: "kMax", 15: "kScale",
			16: "kRelayout", 17: "kSquare", 18: "kInnerLRN",
			19: "kCopy", 20: "kSigmoid", 21: "kTanh",
			22: "kDeconvolution", 23: "kElu", 24: "kReshape",
			25: "kToPlaneMajor", 26: "kPower", 27: "kCrop",
			28: "kTile", 29: "kRegionYolo", 30: "kReorgYolo",
			31: "kConvert_u8f16", 32: "kConvert_f32f16", 33: "kConvert_f16f32",
			34: "kPermute", 35: "kNormalize", 36: "kPriorBox",
			37: "kDetectionOutput", 38: "kMyriadXHwConvolution", 39: "kMyriadXHwPooling",
			40: "kMyriadXHwFCL", 41: "kMyriadXHwPostOps", 42: "kConvertHwSw",
			43: "kCTCDecoder", 44: "kLeakyRelu", 45: "kBiasRelu",
			46: "kBiasLeakyRelu", 47: "kScaleShift", 48: "kCopyMakeBorderCHW",
			49: "kIm2ColConvolution", 50: "kHwFcRelayout",51:"kClamp",52:"kRefConvolution"
		}

	def __init__(self):
		self.nextStage = None
		self.stageType = None
		self.implementation_flag = None
		self.preOpType = None
		self.postOpType = None
		

	def name(self):
		return Stage.STAGE_NAME_TABLE[self.stageType]

	def isHw(self):
		if self.stageType>38:
			return True
		return False


 


class StageSection(object):
	def __init__(self):
		self.stage_count = None
		self.size_of_stage_section = None
		self.size_of_output = None
		self.size_of_input = None
		self.permutation_enabled = None
		self.permutation_size =  None
		self.permutation = list()
		self.stages = list()

class BufferSection(object):
	def __init__(self):
		self.buffer_sec_size = None

	 


class RelocSection(object):
	def __init__(self):
		self.reloc_size = None
		self.blob_reloc_offset = None
		self.blob_reloc_size = None
		self.work_reloc_offset = None
		self.work_reloc_size = None

 
 
class GraphHeader(object):

	 
	def __init__(self):
		self.unused = bytearray(34)
		self.magic = None
		self.filesize = None
		self.blob_vmajor = None
		self.blob_vminor = None
		self.no_shaves = None
		self.stage_section_offset = None
		self.buffer_section_offset = None
		self.reloc_section_offset = None
		self.stage = StageSection()
		self.reloc = RelocSection()
		self.buffer = BufferSection()
 
