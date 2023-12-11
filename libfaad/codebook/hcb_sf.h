/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** The "appropriate copyright message" mentioned in section 2c of the GPLv2
** must read: "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Nero AG through Mpeg4AAClicense@nero.com.
**
** $Id: hcb_sf.h,v 1.7 2007/11/01 12:34:11 menno Exp $
**/

/* Binary search huffman table HCB_SF */


static const uint8_t hcb_sf[241][2] = {
    { /*   0 */  1, 2 },
    { /*   1 */  60, 0 },
    { /*   2 */  1, 2 },
    { /*   3 */  2, 3 },
    { /*   4 */  3, 4 },
    { /*   5 */  59, 0 },
    { /*   6 */  3, 4 },
    { /*   7 */  4, 5 },
    { /*   8 */  5, 6 },
    { /*   9 */  61, 0 },
    { /*  10 */  58, 0 },
    { /*  11 */  62, 0 },
    { /*  12 */  3, 4 },
    { /*  13 */  4, 5 },
    { /*  14 */  5, 6 },
    { /*  15 */  57, 0 },
    { /*  16 */  63, 0 },
    { /*  17 */  4, 5 },
    { /*  18 */  5, 6 },
    { /*  19 */  6, 7 },
    { /*  20 */  7, 8 },
    { /*  21 */  56, 0 },
    { /*  22 */  64, 0 },
    { /*  23 */  55, 0 },
    { /*  24 */  65, 0 },
    { /*  25 */  4, 5 },
    { /*  26 */  5, 6 },
    { /*  27 */  6, 7 },
    { /*  28 */  7, 8 },
    { /*  29 */  66, 0 },
    { /*  30 */  54, 0 },
    { /*  31 */  67, 0 },
    { /*  32 */  5, 6 },
    { /*  33 */  6, 7 },
    { /*  34 */  7, 8 },
    { /*  35 */  8, 9 },
    { /*  36 */  9, 10 },
    { /*  37 */  53, 0 },
    { /*  38 */  68, 0 },
    { /*  39 */  52, 0 },
    { /*  40 */  69, 0 },
    { /*  41 */  51, 0 },
    { /*  42 */  5, 6 },
    { /*  43 */  6, 7 },
    { /*  44 */  7, 8 },
    { /*  45 */  8, 9 },
    { /*  46 */  9, 10 },
    { /*  47 */  70, 0 },
    { /*  48 */  50, 0 },
    { /*  49 */  49, 0 },
    { /*  50 */  71, 0 },
    { /*  51 */  6, 7 },
    { /*  52 */  7, 8 },
    { /*  53 */  8, 9 },
    { /*  54 */  9, 10 },
    { /*  55 */  10, 11 },
    { /*  56 */  11, 12 },
    { /*  57 */  72, 0 },
    { /*  58 */  48, 0 },
    { /*  59 */  73, 0 },
    { /*  60 */  47, 0 },
    { /*  61 */  74, 0 },
    { /*  62 */  46, 0 },
    { /*  63 */  6, 7 },
    { /*  64 */  7, 8 },
    { /*  65 */  8, 9 },
    { /*  66 */  9, 10 },
    { /*  67 */  10, 11 },
    { /*  68 */  11, 12 },
    { /*  69 */  76, 0 },
    { /*  70 */  75, 0 },
    { /*  71 */  77, 0 },
    { /*  72 */  78, 0 },
    { /*  73 */  45, 0 },
    { /*  74 */  43, 0 },
    { /*  75 */  6, 7 },
    { /*  76 */  7, 8 },
    { /*  77 */  8, 9 },
    { /*  78 */  9, 10 },
    { /*  79 */  10, 11 },
    { /*  80 */  11, 12 },
    { /*  81 */  44, 0 },
    { /*  82 */  79, 0 },
    { /*  83 */  42, 0 },
    { /*  84 */  41, 0 },
    { /*  85 */  80, 0 },
    { /*  86 */  40, 0 },
    { /*  87 */  6, 7 },
    { /*  88 */  7, 8 },
    { /*  89 */  8, 9 },
    { /*  90 */  9, 10 },
    { /*  91 */  10, 11 },
    { /*  92 */  11, 12 },
    { /*  93 */  81, 0 },
    { /*  94 */  39, 0 },
    { /*  95 */  82, 0 },
    { /*  96 */  38, 0 },
    { /*  97 */  83, 0 },
    { /*  98 */  7, 8 },
    { /*  99 */  8, 9 },
    { /* 100 */  9, 10 },
    { /* 101 */  10, 11 },
    { /* 102 */  11, 12 },
    { /* 103 */  12, 13 },
    { /* 104 */  13, 14 },
    { /* 105 */  37, 0 },
    { /* 106 */  35, 0 },
    { /* 107 */  85, 0 },
    { /* 108 */  33, 0 },
    { /* 109 */  36, 0 },
    { /* 110 */  34, 0 },
    { /* 111 */  84, 0 },
    { /* 112 */  32, 0 },
    { /* 113 */  6, 7 },
    { /* 114 */  7, 8 },
    { /* 115 */  8, 9 },
    { /* 116 */  9, 10 },
    { /* 117 */  10, 11 },
    { /* 118 */  11, 12 },
    { /* 119 */  87, 0 },
    { /* 120 */  89, 0 },
    { /* 121 */  30, 0 },
    { /* 122 */  31, 0 },
    { /* 123 */  8, 9 },
    { /* 124 */  9, 10 },
    { /* 125 */  10, 11 },
    { /* 126 */  11, 12 },
    { /* 127 */  12, 13 },
    { /* 128 */  13, 14 },
    { /* 129 */  14, 15 },
    { /* 130 */  15, 16 },
    { /* 131 */  86, 0 },
    { /* 132 */  29, 0 },
    { /* 133 */  26, 0 },
    { /* 134 */  27, 0 },
    { /* 135 */  28, 0 },
    { /* 136 */  24, 0 },
    { /* 137 */  88, 0 },
    { /* 138 */  9, 10 },
    { /* 139 */  10, 11 },
    { /* 140 */  11, 12 },
    { /* 141 */  12, 13 },
    { /* 142 */  13, 14 },
    { /* 143 */  14, 15 },
    { /* 144 */  15, 16 },
    { /* 145 */  16, 17 },
    { /* 146 */  17, 18 },
    { /* 147 */  25, 0 },
    { /* 148 */  22, 0 },
    { /* 149 */  23, 0 },
    { /* 150 */  15, 16 },
    { /* 151 */  16, 17 },
    { /* 152 */  17, 18 },
    { /* 153 */  18, 19 },
    { /* 154 */  19, 20 },
    { /* 155 */  20, 21 },
    { /* 156 */  21, 22 },
    { /* 157 */  22, 23 },
    { /* 158 */  23, 24 },
    { /* 159 */  24, 25 },
    { /* 160 */  25, 26 },
    { /* 161 */  26, 27 },
    { /* 162 */  27, 28 },
    { /* 163 */  28, 29 },
    { /* 164 */  29, 30 },
    { /* 165 */  90, 0 },
    { /* 166 */  21, 0 },
    { /* 167 */  19, 0 },
    { /* 168 */   3, 0 },
    { /* 169 */   1, 0 },
    { /* 170 */   2, 0 },
    { /* 171 */   0, 0 },
    { /* 172 */  23, 24 },
    { /* 173 */  24, 25 },
    { /* 174 */  25, 26 },
    { /* 175 */  26, 27 },
    { /* 176 */  27, 28 },
    { /* 177 */  28, 29 },
    { /* 178 */  29, 30 },
    { /* 179 */  30, 31 },
    { /* 180 */  31, 32 },
    { /* 181 */  32, 33 },
    { /* 182 */  33, 34 },
    { /* 183 */  34, 35 },
    { /* 184 */  35, 36 },
    { /* 185 */  36, 37 },
    { /* 186 */  37, 38 },
    { /* 187 */  38, 39 },
    { /* 188 */  39, 40 },
    { /* 189 */  40, 41 },
    { /* 190 */  41, 42 },
    { /* 191 */  42, 43 },
    { /* 192 */  43, 44 },
    { /* 193 */  44, 45 },
    { /* 194 */  45, 46 },
    { /* 195 */   98, 0 },
    { /* 196 */   99, 0 },
    { /* 197 */  100, 0 },
    { /* 198 */  101, 0 },
    { /* 199 */  102, 0 },
    { /* 200 */  117, 0 },
    { /* 201 */   97, 0 },
    { /* 202 */   91, 0 },
    { /* 203 */   92, 0 },
    { /* 204 */   93, 0 },
    { /* 205 */   94, 0 },
    { /* 206 */   95, 0 },
    { /* 207 */   96, 0 },
    { /* 208 */  104, 0 },
    { /* 209 */  111, 0 },
    { /* 210 */  112, 0 },
    { /* 211 */  113, 0 },
    { /* 212 */  114, 0 },
    { /* 213 */  115, 0 },
    { /* 214 */  116, 0 },
    { /* 215 */  110, 0 },
    { /* 216 */  105, 0 },
    { /* 217 */  106, 0 },
    { /* 218 */  107, 0 },
    { /* 219 */  108, 0 },
    { /* 220 */  109, 0 },
    { /* 221 */  118, 0 },
    { /* 222 */    6, 0 },
    { /* 223 */    8, 0 },
    { /* 224 */    9, 0 },
    { /* 225 */   10, 0 },
    { /* 226 */    5, 0 },
    { /* 227 */  103, 0 },
    { /* 228 */  120, 0 },
    { /* 229 */  119, 0 },
    { /* 230 */    4, 0 },
    { /* 231 */    7, 0 },
    { /* 232 */   15, 0 },
    { /* 233 */   16, 0 },
    { /* 234 */   18, 0 },
    { /* 235 */   20, 0 },
    { /* 236 */   17, 0 },
    { /* 237 */   11, 0 },
    { /* 238 */   12, 0 },
    { /* 239 */   14, 0 },
    { /* 240 */   13, 0 }
};
