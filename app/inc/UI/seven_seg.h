#ifndef SEVEN_SEG_H
#define SEVEN_SEG_H

#include <zephyr/drivers/led.h>
#include <stdint.h>
#include <zephyr/logging/log.h>

static const uint8_t siekoo_seg[36] = {
    /* digits */
    0x3F,  /* 0  abcdef  */
    0x06,  /* 1  bc      */
    0x5B,  /* 2  abdeg   */
    0x4F,  /* 3  abcdg   */
    0x66,  /* 4  bcfg    */
    0x6D,  /* 5  acdfg   */
    0x7D,  /* 6  acdefg  */
    0x07,  /* 7  abc     */
    0x7F,  /* 8  abcdefg */
    0x6F,  /* 9  abcdfg  */
 
    /* letters */
    0x5F,  /* a  abcdeg  */
    0x7C,  /* b  cdefg   */
    0x58,  /* c  deg     */
    0x5E,  /* d  bcdeg   */
    0x79,  /* e  adefg   */
    0x71,  /* f  aefg    */
    0x3D,  /* g  acdef   */
    0x74,  /* h  cefg    */
    0x11,  /* i  ae      */
    0x0D,  /* j  acd     */
    0x75,  /* k  acefg   */
    0x38,  /* l  def     */
    0x55,  /* m  aceg    */
    0x54,  /* n  ceg     */
    0x5C,  /* o  cdeg    */
    0x73,  /* p  abefg   */
    0x67,  /* q  abcfg   */
    0x50,  /* r  eg      */
    0x2D,  /* s  acdf    */
    0x78,  /* t  defg    */
    0x1C,  /* u  cde     */
    0x2A,  /* v  bdf     */
    0x6A,  /* w  bdfg    */
    0x14,  /* x  ce      */
    0x6E,  /* y  bcdfg   */
    0x1B,  /* z  abde    */
};

static const uint8_t digit_offset[4] = { //Which of the 4 digits to put the number, color is 33
    0,
    16, 
    48, 
    64,
};

int display_seven_seg(const struct device *dev, char* chars, size_t size);
int display_seven_seg_punctuation(const struct device *dev, uint8_t punc_index);
int clear_seven_seg(const struct device *dev);


#endif // SEVEN_SEG_H