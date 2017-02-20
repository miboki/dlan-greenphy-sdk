/*
 * @brief Endianness declaration
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * Copyright(C) Dean Camera, 2011, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */



#ifndef __LPCUSBlib_ENDIANNESS_H__
#define __LPCUSBlib_ENDIANNESS_H__

  /* Enable C linkage for C++ Compilers: */
  #if defined(__cplusplus)
   extern "C" {
  #endif

 /* Preprocessor Checks: */
  #if !(defined(ARCH_BIG_ENDIAN) || defined(ARCH_LITTLE_ENDIAN))
   #error ARCH_BIG_ENDIAN or ARCH_LITTLE_ENDIAN not set for the specified architecture.
  #endif

 /* Public Interface - May be used in end-application: */
  /* Macros: */
   #define SWAPENDIAN_16(x)            (uint16_t)((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))

   #define SWAPENDIAN_32(x)            (uint32_t)((((x) & 0xFF000000UL) >> 24UL) | (((x) & 0x00FF0000UL) >> 8UL) | \
                                                  (((x) & 0x0000FF00UL) << 8UL)  | (((x) & 0x000000FFUL) << 24UL))

   #if defined(ARCH_BIG_ENDIAN) && !defined(le16_to_cpu)
    #define le16_to_cpu(x)           SwapEndian_16(x)
    #define le32_to_cpu(x)           SwapEndian_32(x)
    #define be16_to_cpu(x)           (x)
    #define be32_to_cpu(x)           (x)
    #define cpu_to_le16(x)           SwapEndian_16(x)
    #define cpu_to_le32(x)           SwapEndian_32(x)
    #define cpu_to_be16(x)           (x)
    #define cpu_to_be32(x)           (x)
    #define LE16_TO_CPU(x)           SWAPENDIAN_16(x)
    #define LE32_TO_CPU(x)           SWAPENDIAN_32(x)
    #define BE16_TO_CPU(x)           (x)
    #define BE32_TO_CPU(x)           (x)
    #define CPU_TO_LE16(x)           SWAPENDIAN_16(x)
    #define CPU_TO_LE32(x)           SWAPENDIAN_32(x)
    #define CPU_TO_BE16(x)           (x)
    #define CPU_TO_BE32(x)           (x)
   #elif !defined(le16_to_cpu)


    #define le16_to_cpu(x)           (x)

    #define le32_to_cpu(x)           (x)

    #define be16_to_cpu(x)           SwapEndian_16(x)

    #define be32_to_cpu(x)           SwapEndian_32(x)

    #define cpu_to_le16(x)           (x)

    #define cpu_to_le32(x)           (x)

    #define cpu_to_be16(x)           SwapEndian_16(x)

    #define cpu_to_be32(x)           SwapEndian_32(x)



    #define LE16_TO_CPU(x)           (x)

    #define LE32_TO_CPU(x)           (x)

    #define BE16_TO_CPU(x)           SWAPENDIAN_16(x)

    #define BE32_TO_CPU(x)           SWAPENDIAN_32(x)

    #define CPU_TO_LE16(x)           (x)

    #define CPU_TO_LE32(x)           (x)

    #define CPU_TO_BE16(x)           SWAPENDIAN_16(x)

    #define CPU_TO_BE32(x)           SWAPENDIAN_32(x)

   #endif

  /* Inline Functions: */
   static inline uint16_t SwapEndian_16(const uint16_t Word) ATTR_WARN_UNUSED_RESULT ATTR_CONST;
   static inline uint16_t SwapEndian_16(const uint16_t Word)
   {
    if (GCC_IS_COMPILE_CONST(Word))
      return SWAPENDIAN_16(Word);

    uint8_t Temp;

    union
    {
     uint16_t Word;
     uint8_t  Bytes[2];
    } Data;

    Data.Word = Word;

    Temp = Data.Bytes[0];
    Data.Bytes[0] = Data.Bytes[1];
    Data.Bytes[1] = Temp;

    return Data.Word;
   }

   static inline uint32_t SwapEndian_32(const uint32_t DWord) ATTR_WARN_UNUSED_RESULT ATTR_CONST;
   static inline uint32_t SwapEndian_32(const uint32_t DWord)
   {
    if (GCC_IS_COMPILE_CONST(DWord))
      return SWAPENDIAN_32(DWord);

    uint8_t Temp;

    union
    {
     uint32_t DWord;
     uint8_t  Bytes[4];
    } Data;

    Data.DWord = DWord;

    Temp = Data.Bytes[0];
    Data.Bytes[0] = Data.Bytes[3];
    Data.Bytes[3] = Temp;

    Temp = Data.Bytes[1];
    Data.Bytes[1] = Data.Bytes[2];
    Data.Bytes[2] = Temp;

    return Data.DWord;
   }

   static inline void SwapEndian_n(void* const Data,
                                   uint8_t Length) ATTR_NON_NULL_PTR_ARG(1);
   static inline void SwapEndian_n(void* const Data,
                                   uint8_t Length)
   {
    uint8_t* CurrDataPos = (uint8_t*)Data;

    while (Length > 1)
    {
     uint8_t Temp = *CurrDataPos;
     *CurrDataPos = *(CurrDataPos + Length - 1);
     *(CurrDataPos + Length - 1) = Temp;

     CurrDataPos++;
     Length -= 2;
    }
   }

 /* Disable C linkage for C++ Compilers: */
  #if defined(__cplusplus)
   }
  #endif

#endif
