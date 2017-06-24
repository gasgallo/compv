/* Copyright (C) 2016-2017 Doubango Telecom <https://www.doubango.org>
* File author: Mamadou DIOP (Doubango Telecom, France).
* License: GPLv3. For commercial license please contact us.
* Source code: https://github.com/DoubangoTelecom/compv
* WebSite: http://compv.org
*/
#include "compv/base/image/intrin/x86/compv_image_conv_to_rgbx_intrin_sse2.h"

#if COMPV_ARCH_X86 && COMPV_INTRINSIC
#include "compv/base/intrin/x86/compv_intrin_sse.h"
#include "compv/base/image/compv_image_conv_common.h"
#include "compv/base/compv_simd_globals.h"
#include "compv/base/math/compv_math.h"
#include "compv/base/compv_debug.h"

COMPV_NAMESPACE_BEGIN()

void CompVImageConvYuv420_to_Rgba32_Intrin_SSE2(COMPV_ALIGNED(SSE) const uint8_t* yPtr, COMPV_ALIGNED(SSE) const uint8_t* uPtr, COMPV_ALIGNED(SSE) const uint8_t* vPtr, COMPV_ALIGNED(SSE) uint8_t* rgbaPtr, compv_uscalar_t width, compv_uscalar_t height, COMPV_ALIGNED(SSE) compv_uscalar_t stride)
{
	COMPV_DEBUG_INFO_CHECK_SSE2();

	compv_uscalar_t i, j, k, l;
	const compv_uscalar_t strideUV = ((stride + 1) >> 1);
	const compv_uscalar_t strideRGBA = (stride << 2);
	__m128i vecYlow, vecYhigh, vecU, vecV, vecR, vecG, vecB;
	__m128i vec0, vec1;
	const __m128i vecZero = _mm_setzero_si128();
	const __m128i vec16 = _mm_set1_epi16(16);
	const __m128i vec37 = _mm_set1_epi16(37);
	const __m128i vec51 = _mm_set1_epi16(51);
	const __m128i vec65 = _mm_set1_epi16(65);
	const __m128i vec127 = _mm_set1_epi16(127);
	const __m128i vec13_26 = _mm_set1_epi32(0x001a000d); // 13, 26, 13, 26 ...
	const __m128i vecA = _mm_cmpeq_epi8(vec127, vec127); // 255, 255, 255, 255

	// ASM code do not create variables for k and l: k = [(i<<2)] and l = [i>>1]

	for (j = 0; j < height; ++j) {
		for (i = 0, k = 0, l = 0; i < width; i += 16, k += 64, l += 8) {
			/* Load samples */
			vecYlow = _mm_load_si128(reinterpret_cast<const __m128i*>(&yPtr[i])); // #16 Y samples
			vecU = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&uPtr[l])); // #8 U samples, low mem
			vecV = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&vPtr[l])); // #8 V samples, low mem

			/* == Staring this line we're just converting from Y,U,V to R,G,B == */

			/* Convert to I16 */
			vecYhigh = _mm_unpackhi_epi8(vecYlow, vecZero);
			vecYlow = _mm_unpacklo_epi8(vecYlow, vecZero);
			vecU = _mm_unpacklo_epi8(vecU, vecZero);
			vecV = _mm_unpacklo_epi8(vecV, vecZero);

			/* Compute Y', U', V' */
			vecYlow = _mm_sub_epi16(vecYlow, vec16);
			vecYhigh = _mm_sub_epi16(vecYhigh, vec16);
			vecU = _mm_sub_epi16(vecU, vec127);
			vecV = _mm_sub_epi16(vecV, vec127);

			/* Compute (37Y'), (51V') and (65U') */
			vecYlow = _mm_mullo_epi16(vec37, vecYlow);
			vecYhigh = _mm_mullo_epi16(vec37, vecYhigh);
			vec0 = _mm_mullo_epi16(vec51, vecV);
			vec1 = _mm_mullo_epi16(vec65, vecU);

			/* Compute R = (37Y' + 0U' + 51V') >> 5 */
			vecR = _mm_packus_epi16(
				_mm_srai_epi16(_mm_add_epi16(vecYlow, _mm_unpacklo_epi16(vec0, vec0)), 5),
				_mm_srai_epi16(_mm_add_epi16(vecYhigh, _mm_unpackhi_epi16(vec0, vec0)), 5)
			);

			/* B = (37Y' + 65U' + 0V') >> 5 */
			vecB = _mm_packus_epi16(
				_mm_srai_epi16(_mm_add_epi16(vecYlow, _mm_unpacklo_epi16(vec1, vec1)), 5),
				_mm_srai_epi16(_mm_add_epi16(vecYhigh, _mm_unpackhi_epi16(vec1, vec1)), 5)
			);

			/* Compute G = (37Y' - 13U' - 26V') >> 5 = (37Y' - (13U' + 26V')) >> 5 */
			vec0 = _mm_madd_epi16(vec13_26, _mm_unpacklo_epi16(vecU, vecV)); // (13U' + 26V').low - I32
			vec1 = _mm_madd_epi16(vec13_26, _mm_unpackhi_epi16(vecU, vecV)); // (13U' + 26V').high - I32
			vec0 = _mm_packs_epi32(vec0, vec1);
			vecG = _mm_packus_epi16(
				_mm_srai_epi16(_mm_sub_epi16(vecYlow, _mm_unpacklo_epi16(vec0, vec0)), 5),
				_mm_srai_epi16(_mm_sub_epi16(vecYhigh, _mm_unpackhi_epi16(vec0, vec0)), 5)
			);
			
			/* Store result */
			COMPV_VST4_I8_SSSE3(&rgbaPtr[k], vecR, vecG, vecB, vecA, vec0, vec1);

		} // End_Of for (i = 0; i < width; i += 16)
		yPtr += stride;
		rgbaPtr += strideRGBA;
		if (j & 1) {
			uPtr += strideUV;
			vPtr += strideUV;
		} // End_Of for (j = 0; j < height; ++j)
	}
}

COMPV_NAMESPACE_END()

#endif /* COMPV_ARCH_X86 && COMPV_INTRINSIC */