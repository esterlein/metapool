#pragma once

#include "mtp/metaset.hpp"


namespace mtp {


//---------------------------------------------------------
// use default set or add set definitions
//---------------------------------------------------------


using default_set = metaset <

	def<capf::mul2, 1024,      32,      32,     512,  2016>,
	def<capf::mul2,  512,     512,    2048,    8192, 32256>,
	def<capf::flat,  128,    8192,   32768,  122880>,
	def<capf::flat,   64,  131072,  131072,  917504>,
	def<capf::flat,   32, 1048576, 1048576, 8388608>

>;


} // mtp
