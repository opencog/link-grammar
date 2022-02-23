#!/usr/bin/env python
#-*- coding: utf-8 -*-

import os, sys

############################################################

def read_content(words, nes, starts, fname):
    fhdl = open(fname)
    
    for line in fhdl:
        line = line.strip()
        if len(line) == 0: continue

        if line.count('.') > 1 or line.endswith('.') or len(line) > 20 or line.count('_') > 0:
            nes.add(line)
        elif match_start(line, starts):
            nes.add(line)
        else:
            words.add(line)
    
    fhdl.close()

############################################################

def match_start(text, starts):
    for start in starts:
        if text.startswith(start) and len(text) > len(start):
            return True
    return False

############################################################

def main():
    words, nes = set(), set()
    starts = ['กรม', 'กรรมการ', 'กระทรวง', 'กรุง', 'กลุ่ม', 'กอง', 'การบิน', 'การไฟฟ้า', 'ขุน', 'คณะ', 'คลอง', 'จังหวัด', 'ชุมชน', 'ซอย', 'ดอน', 'ดอย', 'ตลาด', 'ตำบล', 'ตำรวจ', 'ถนน', 'ทะเล', 'ธนาคาร', 'นคร', 'บ้าน', 'ประเทศ', 'ฝ่ายคดี', 'พรรค', 'มหาวิทยาลัย', 'มูลนิธิ', 'รัฐ', 'ร้าน', 'ศาล', 'ศูนย์', 'สถานี', 'สถาบัน', 'สนามกีฬา', 'สนามบิน', 'สมาคม', 'สมเด็จ', 'สวน', 'สาธารณรัฐ', 'สำนักงาน', 'สโมสร', 'หนอง', 'หนังสือพิมพ์', 'หมู่เกาะ', 'หลวงพ่อ', 'ห้วย', 'องค์การ', 'องค์กร', 'อาคาร', 'อำเภอ', 'เกาะ', 'เขต', 'เขา', 'เจ้าฟ้า', 'เดอะ', 'เทคโนโลยี', 'เทคโนฯ', 'เทศบาล', 'แขวง', 'แซ่', 'แม่น้ำ', 'โรงพยาบาล', 'โรงเรียน', 'โรงแรม']

    for fname in sys.argv[1:]:
        read_content(words, nes, starts, fname)

    print('**** WORDS ****')
    for word in sorted(words):
        print(word)

    print('\n**** POTENTIAL NEs ****')
    for word in sorted(nes):
        print(word)

############################################################

if __name__ == '__main__':
    main()
