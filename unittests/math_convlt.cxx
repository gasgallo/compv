#include "../tests/tests_common.h"

#define TAG_TEST								"UnitTestMathConvolution"

static const struct compv_unittest_convlt {
	const bool fixedPoint;
	const COMPV_SUBTYPE inputType;
	const COMPV_SUBTYPE KernelType;
	const COMPV_SUBTYPE OutputType;
	const size_t data_width;
	const size_t data_height;
	const size_t data_stride;
	const size_t kernel_width;
	const size_t kernel_height;
	const char* md5;
	const char* md5_fma;
} COMPV_UNITTEST_CONVLT[] =
{
	{ true, COMPV_SUBTYPE_RAW_UINT8, COMPV_SUBTYPE_RAW_UINT16, COMPV_SUBTYPE_RAW_UINT8, 1282, 720, 1344, 7, 7, "6104137e8ad0acedc521066e8c18aae3" }, // FixedPoint - 0
	{ false, COMPV_SUBTYPE_RAW_UINT8, COMPV_SUBTYPE_RAW_FLOAT32, COMPV_SUBTYPE_RAW_UINT8, 1282, 720, 1344, 7, 7, "4cedd0ff1838e9f4a406084af06ee1f9", "4cedd0ff1838e9f4a406084af06ee1f9" }, // FloatingPoint - 1
	{ false, COMPV_SUBTYPE_RAW_UINT8, COMPV_SUBTYPE_RAW_FLOAT32, COMPV_SUBTYPE_RAW_FLOAT32, 1282, 720, 1344, 7, 7, "de06034aaee56a3cf150411c5522cf60", "de06034aaee56a3cf150411c5522cf60" }, // FloatingPoint - 2
	{ false, COMPV_SUBTYPE_RAW_FLOAT32, COMPV_SUBTYPE_RAW_FLOAT32, COMPV_SUBTYPE_RAW_FLOAT32, 1282, 720, 1344, 7, 7, "5f72ff619e2e86fcd9211edab1d94b1c", "5f72ff619e2e86fcd9211edab1d94b1c" }, // FloatingPoint - 3
	{ false, COMPV_SUBTYPE_RAW_FLOAT32, COMPV_SUBTYPE_RAW_FLOAT32, COMPV_SUBTYPE_RAW_UINT8, 1282, 720, 1344, 7, 7, "bb404ba161a09a77b8021fe0aeb193b0", "bb404ba161a09a77b8021fe0aeb193b0" }, // FloatingPoint - 4
	{ false, COMPV_SUBTYPE_RAW_UINT8, COMPV_SUBTYPE_RAW_INT16, COMPV_SUBTYPE_RAW_INT16, 1282, 720, 1344, 7, 7, "07ee2ea9579b6104d5136e1bbd9f2cf1" }, // Integer - 5
	{ false, COMPV_SUBTYPE_RAW_INT16, COMPV_SUBTYPE_RAW_INT16, COMPV_SUBTYPE_RAW_INT16, 1282, 720, 1344, 7, 7, "2844cd36948f2ef8589a757efbaef691" }, // Integer - 6
};
static const size_t COMPV_UNITTEST_CONVLT_COUNT = sizeof(COMPV_UNITTEST_CONVLT) / sizeof(COMPV_UNITTEST_CONVLT[0]);

static COMPV_ERROR_CODE buildTest(const compv_unittest_convlt* test, CompVMatPtrPtr data, CompVMatPtrPtr kernel);

#define convolution(inputType, KernelType, OutputType) { \
	COMPV_CHECK_CODE_RETURN(CompVMat::newObjAligned<OutputType>(&dataOut, dataIn->rows(), dataIn->cols(), dataIn->stride())); \
	OutputType* outPtr = dataOut->ptr<OutputType>(); \
	COMPV_CHECK_CODE_RETURN(CompVMathConvlt::convlt1(dataIn->ptr<const inputType>(), dataIn->cols(), dataIn->rows(), dataIn->stride(), \
		kernel->ptr<const KernelType>(), kernel->ptr<const KernelType>(), test->kernel_width, outPtr \
	)); \
}

static const std::string compv_unittest_convlt_to_string(const compv_unittest_convlt* test, const bool fma) {
	return
		std::string("FMA:") + CompVBase::to_string(fma) + std::string(", ")
		+ std::string("fixedPoint:") + CompVBase::to_string(test->fixedPoint) + std::string(", ")
		+ std::string("inputType:") + std::string(CompVGetSubtypeString(test->inputType)) + std::string(", ")
		+ std::string("KernelType:") + std::string(CompVGetSubtypeString(test->KernelType)) + std::string(", ")
		+ std::string("OutputType:") + std::string(CompVGetSubtypeString(test->OutputType));
}

COMPV_ERROR_CODE unittest_math_convlt()
{
	for (size_t i = 0; i < COMPV_UNITTEST_CONVLT_COUNT; ++i) {
		const compv_unittest_convlt* test = &COMPV_UNITTEST_CONVLT[i];

		const bool fma = (test->KernelType == COMPV_SUBTYPE_RAW_FLOAT32) && compv_tests_is_fma_enabled();

		COMPV_DEBUG_INFO_EX(TAG_TEST, "== Trying new test: convlt -> %s ==", compv_unittest_convlt_to_string(test, fma).c_str());

		// Build test
		CompVMatPtr dataIn, kernel;
		COMPV_CHECK_CODE_RETURN(buildTest(test, &dataIn, &kernel));

		// Perform convolution
		CompVMatPtr dataOut;
		if (test->fixedPoint && test->inputType == COMPV_SUBTYPE_RAW_UINT8 && test->KernelType == COMPV_SUBTYPE_RAW_UINT16 && test->OutputType == COMPV_SUBTYPE_RAW_UINT8) {
			COMPV_CHECK_CODE_RETURN(CompVMat::newObjAligned<uint8_t>(&dataOut, dataIn->rows(), dataIn->cols(), dataIn->stride()));
			uint8_t* outPtr = dataOut->ptr<uint8_t>();
			COMPV_CHECK_CODE_RETURN(CompVMathConvlt::convlt1FixedPoint(dataIn->ptr<const uint8_t>(), dataIn->cols(), dataIn->rows(), dataIn->stride(),
				kernel->ptr<const uint16_t>(), kernel->ptr<const uint16_t>(), test->kernel_width, outPtr
			));
		}
		else if (!test->fixedPoint && test->inputType == COMPV_SUBTYPE_RAW_UINT8 && test->KernelType == COMPV_SUBTYPE_RAW_FLOAT32 && test->OutputType == COMPV_SUBTYPE_RAW_UINT8) {
			convolution(uint8_t, compv_float32_t, uint8_t);
		}
		else if (!test->fixedPoint && test->inputType == COMPV_SUBTYPE_RAW_UINT8 && test->KernelType == COMPV_SUBTYPE_RAW_FLOAT32 && test->OutputType == COMPV_SUBTYPE_RAW_FLOAT32) {
			convolution(uint8_t, compv_float32_t, compv_float32_t);
		}
		else if (!test->fixedPoint && test->inputType == COMPV_SUBTYPE_RAW_FLOAT32 && test->KernelType == COMPV_SUBTYPE_RAW_FLOAT32 && test->OutputType == COMPV_SUBTYPE_RAW_FLOAT32) {
			convolution(compv_float32_t, compv_float32_t, compv_float32_t);
		}
		else if (!test->fixedPoint && test->inputType == COMPV_SUBTYPE_RAW_FLOAT32 && test->KernelType == COMPV_SUBTYPE_RAW_FLOAT32 && test->OutputType == COMPV_SUBTYPE_RAW_UINT8) {
			convolution(compv_float32_t, compv_float32_t, uint8_t);
		}
		else if (!test->fixedPoint && test->inputType == COMPV_SUBTYPE_RAW_UINT8 && test->KernelType == COMPV_SUBTYPE_RAW_INT16 && test->OutputType == COMPV_SUBTYPE_RAW_INT16) {
			convolution(uint8_t, int16_t, int16_t);
		}
		else if (!test->fixedPoint && test->inputType == COMPV_SUBTYPE_RAW_INT16 && test->KernelType == COMPV_SUBTYPE_RAW_INT16 && test->OutputType == COMPV_SUBTYPE_RAW_INT16) {
			convolution(int16_t, int16_t, int16_t);
		}
		else {
			COMPV_CHECK_CODE_RETURN(COMPV_ERROR_CODE_E_NOT_IMPLEMENTED);
		}

		COMPV_CHECK_EXP_RETURN(std::string(fma ? test->md5_fma : test->md5).compare(compv_tests_md5(dataOut)) != 0, COMPV_ERROR_CODE_E_UNITTEST_FAILED, "Convlt MD5 mismatch");
	}

	COMPV_DEBUG_INFO_EX(TAG_TEST, "** Test OK **");
	return COMPV_ERROR_CODE_S_OK;
}

static COMPV_ERROR_CODE buildTest(const compv_unittest_convlt* test, CompVMatPtrPtr data, CompVMatPtrPtr kernel)
{
	switch (test->inputType) {
	case COMPV_SUBTYPE_RAW_UINT8: {
		COMPV_CHECK_CODE_RETURN((CompVMat::newObjAligned<uint8_t>(data, test->data_height, test->data_width, test->data_stride)));
		uint8_t* ptr8u = (*data)->ptr<uint8_t>();
		for (size_t j = 0; j < test->data_height; ++j) {
			for (size_t i = 0; i < test->data_width; ++i) {
				ptr8u[i] = static_cast<uint8_t>((i * j) + 53);
			}
			ptr8u += test->data_stride;
		}
		break;
	}
	case COMPV_SUBTYPE_RAW_FLOAT32: {
		COMPV_CHECK_CODE_RETURN((CompVMat::newObjAligned<compv_float32_t>(data, test->data_height, test->data_width, test->data_stride)));
		compv_float32_t* ptr32f = (*data)->ptr<compv_float32_t>();
		for (size_t j = 0; j < test->data_height; ++j) {
			for (size_t i = 0; i < test->data_width; ++i) {
				ptr32f[i] = static_cast<compv_float32_t>(((i * j) + 53.558f) * (((i * j) & 1) ? 1.f : -1.f)) / 1.2f; // make sure to have negative values
			}
			ptr32f += test->data_stride;
		}
		break;
	}
	case COMPV_SUBTYPE_RAW_INT16: {
		COMPV_CHECK_CODE_RETURN((CompVMat::newObjAligned<int16_t>(data, test->data_height, test->data_width, test->data_stride)));
		int16_t* ptr16s = (*data)->ptr<int16_t>();
		for (size_t j = 0; j < test->data_height; ++j) {
			for (size_t i = 0; i < test->data_width; ++i) {
				ptr16s[i] = static_cast<int16_t>(((i * j) + 53) * ((i & 1) ? -1 : 1));
			}
			ptr16s += test->data_stride;
		}
		break;
	}
	default: {
		COMPV_CHECK_CODE_RETURN(COMPV_ERROR_CODE_E_NOT_IMPLEMENTED);
		break;
	}
	}

	if (test->fixedPoint) {
		COMPV_CHECK_CODE_RETURN(CompVMathGauss::kernelDim1(kernel, test->kernel_width, 3.5));
	}
	else {
		switch (test->KernelType) {
		case COMPV_SUBTYPE_RAW_FLOAT32: {
			COMPV_CHECK_CODE_RETURN((CompVMat::newObjAligned<compv_float32_t>(kernel, 1, test->kernel_width)));
			compv_float32_t* ptr32f = (*kernel)->ptr<compv_float32_t>();
			for (size_t i = 0; i < test->kernel_width; ++i) {
				ptr32f[i] = static_cast<compv_float32_t>(((i * 1.f) + 53.558f) * (((i * 1) & 1) ? 1.f : -1.f)) / 1.2f; // make sure to have negative values
			}
			break;
		}
		case COMPV_SUBTYPE_RAW_INT16: {
			COMPV_CHECK_CODE_RETURN((CompVMat::newObjAligned<int16_t>(kernel, 1, test->kernel_width)));
			int16_t* ptr16s = (*kernel)->ptr<int16_t>();
			for (size_t i = 0; i < test->kernel_width; ++i) {
				ptr16s[i] = static_cast<int16_t>(((i * 1) + 53) * ((i & 1) ? -1 : 1));
			}
			break;
		}
		default: {
			COMPV_CHECK_CODE_RETURN(COMPV_ERROR_CODE_E_NOT_IMPLEMENTED);
			break;
		}
		}
	}

	return COMPV_ERROR_CODE_S_OK;
}
