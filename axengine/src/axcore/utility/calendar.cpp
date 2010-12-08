﻿/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#include "../private.h"

AX_BEGIN_NAMESPACE


/***************************************************************************
致看到这些源代码的兄弟:
你好!
这本来是我为一个商业PDA产品开发的日历程序,最近移植于PC机上, 所以算法  
和数据部分是用纯C++写的,不涉及MFC,所有的代码都是以短节省存储空间为主要目的.  

很高兴你对这些代码有兴趣,你可以随意复制和使用些代码,唯一有一点小小的  
愿望:在你使用和复制给别人时,别忘注明这些代码作者:-)。程序代码也就罢了,后  
面的数据可是我辛辛苦苦从万年历上找出来输进去的。  
如果你有什么好的意见不妨Mail给我。  

wangfei@hanwang.com.cn  
或  
wangfei@engineer.com.cn  
2000年3月  
****************************************************************************/

/******************************************************************************  
下面为阴历计算所需的数据,为节省存储空间,所以采用下面比较变态的存储方法.  
*******************************************************************************/  
// 数组gLunarDay存入阴历1901年到2100年每年中的月天数信息，  
// 阴历每月只能是29或30天，一年用12（或13）个二进制位表示，对应位为1表30天，否则为29天  
static ushort_t gLunarMonthDay[] = {  
	//测试数据只有1901.1.1 --2050.12.31  
	0x4ae0, 0xa570, 0x5268, 0xd260, 0xd950, 0x6aa8, 0x56a0, 0x9ad0, 0x4ae8, 0x4ae0,   //1910  
	0xa4d8, 0xa4d0, 0xd250, 0xd548, 0xb550, 0x56a0, 0x96d0, 0x95b0, 0x49b8, 0x49b0,   //1920  
	0xa4b0, 0xb258, 0x6a50, 0x6d40, 0xada8, 0x2b60, 0x9570, 0x4978, 0x4970, 0x64b0,   //1930  
	0xd4a0, 0xea50, 0x6d48, 0x5ad0, 0x2b60, 0x9370, 0x92e0, 0xc968, 0xc950, 0xd4a0,   //1940  
	0xda50, 0xb550, 0x56a0, 0xaad8, 0x25d0, 0x92d0, 0xc958, 0xa950, 0xb4a8, 0x6ca0,   //1950  
	0xb550, 0x55a8, 0x4da0, 0xa5b0, 0x52b8, 0x52b0, 0xa950, 0xe950, 0x6aa0, 0xad50,   //1960  
	0xab50, 0x4b60, 0xa570, 0xa570, 0x5260, 0xe930, 0xd950, 0x5aa8, 0x56a0, 0x96d0,   //1970  
	0x4ae8, 0x4ad0, 0xa4d0, 0xd268, 0xd250, 0xd528, 0xb540, 0xb6a0, 0x96d0, 0x95b0,   //1980  
	0x49b0, 0xa4b8, 0xa4b0, 0xb258, 0x6a50, 0x6d40, 0xada0, 0xab60, 0x9370, 0x4978,   //1990  
	0x4970, 0x64b0, 0x6a50, 0xea50, 0x6b28, 0x5ac0, 0xab60, 0x9368, 0x92e0, 0xc960,   //2000  
	0xd4a8, 0xd4a0, 0xda50, 0x5aa8, 0x56a0, 0xaad8, 0x25d0, 0x92d0, 0xc958, 0xa950,   //2010  
	0xb4a0, 0xb550, 0xb550, 0x55a8, 0x4ba0, 0xa5b0, 0x52b8, 0x52b0, 0xa930, 0x74a8,   //2020  
	0x6aa0, 0xad50, 0x4da8, 0x4b60, 0x9570, 0xa4e0, 0xd260, 0xe930, 0xd530, 0x5aa0,   //2030  
	0x6b50, 0x96d0, 0x4ae8, 0x4ad0, 0xa4d0, 0xd258, 0xd250, 0xd520, 0xdaa0, 0xb5a0,   //2040  
	0x56d0, 0x4ad8, 0x49b0, 0xa4b8, 0xa4b0, 0xaa50, 0xb528, 0x6d20, 0xada0, 0x55b0,   //2050  
};  

// 数组gLanarMonth存放阴历1901年到2050年闰月的月份，如没有则为0，每字节存两年  
static byte_t  gLunarMonth[] = {  
	0x00, 0x50, 0x04, 0x00, 0x20,   //1910  
	0x60, 0x05, 0x00, 0x20, 0x70,   //1920  
	0x05, 0x00, 0x40, 0x02, 0x06,   //1930  
	0x00, 0x50, 0x03, 0x07, 0x00,   //1940  
	0x60, 0x04, 0x00, 0x20, 0x70,   //1950  
	0x05, 0x00, 0x30, 0x80, 0x06,   //1960  
	0x00, 0x40, 0x03, 0x07, 0x00,   //1970  
	0x50, 0x04, 0x08, 0x00, 0x60,   //1980  
	0x04, 0x0a, 0x00, 0x60, 0x05,   //1990  
	0x00, 0x30, 0x80, 0x05, 0x00,   //2000  
	0x40, 0x02, 0x07, 0x00, 0x50,   //2010  
	0x04, 0x09, 0x00, 0x60, 0x04,   //2020  
	0x00, 0x20, 0x60, 0x05, 0x00,   //2030  
	0x30, 0xb0, 0x06, 0x00, 0x50,   //2040  
	0x02, 0x07, 0x00, 0x50, 0x03    //2050  
};  

// 数组gLanarHoliDay存放每年的二十四节气对应的阳历日期  
// 每年的二十四节气对应的阳历日期几乎固定，平均分布于十二个月中

//   1月          2月         3月         4月         5月         6月 
// 小寒 大寒   立春  雨水   惊蛰 春分   清明 谷雨   立夏 小满   芒种 夏至

//   7月          8月         9月         10月       11月        12月 
// 小暑 大暑   立秋  处暑   白露 秋分   寒露 霜降   立冬 小雪   大雪 冬至  

/*********************************************************************************  
节气无任何确定规律,所以只好存表,要节省空间,所以....  
下面这种存法实在是太变态了,你就将就着看吧  
**********************************************************************************/  
// 数据格式说明:  
// 如1901年的节气为  
//  1月     2月     3月   4月    5月   6月   7月    8月   9月    10月  11月     12月  
// 6, 21, 4, 19,  6, 21, 5, 21, 6,22, 6,22, 8, 23, 8, 24, 8, 24, 8, 24, 8, 23, 8, 22  
// 9, 6,  11,4,   9, 6,  10,6,  9,7,  9,7,  7, 8,  7, 9,  7,  9, 7,  9, 7,  8, 7, 15  
// 上面第一行数据为每月节气对应日期,15减去每月第一个节气,每月第二个节气减去15得第二行  
//  这样每月两个节气对应数据都小于16,每月用一个字节存放,高位存放第一个节气数据,低位存放  
// 第二个节气的数据,可得下表  

static byte_t gLunarHolDay[] = {  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1901  
	0x96, 0xA4, 0x96, 0x96, 0x97, 0x87, 0x79, 0x79, 0x79, 0x69, 0x78, 0x78,   //1902  
	0x96, 0xA5, 0x87, 0x96, 0x87, 0x87, 0x79, 0x69, 0x69, 0x69, 0x78, 0x78,   //1903  
	0x86, 0xA5, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x79, 0x78, 0x87,   //1904  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1905  
	0x96, 0xA4, 0x96, 0x96, 0x97, 0x97, 0x79, 0x79, 0x79, 0x69, 0x78, 0x78,   //1906  
	0x96, 0xA5, 0x87, 0x96, 0x87, 0x87, 0x79, 0x69, 0x69, 0x69, 0x78, 0x78,   //1907  
	0x86, 0xA5, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //1908  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1909  
	0x96, 0xA4, 0x96, 0x96, 0x97, 0x97, 0x79, 0x79, 0x79, 0x69, 0x78, 0x78,   //1910  
	0x96, 0xA5, 0x87, 0x96, 0x87, 0x87, 0x79, 0x69, 0x69, 0x69, 0x78, 0x78,   //1911  
	0x86, 0xA5, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //1912  
	0x95, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1913  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x79, 0x79, 0x79, 0x69, 0x78, 0x78,   //1914  
	0x96, 0xA5, 0x97, 0x96, 0x97, 0x87, 0x79, 0x79, 0x69, 0x69, 0x78, 0x78,   //1915  
	0x96, 0xA5, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x79, 0x77, 0x87,   //1916  
	0x95, 0xB4, 0x96, 0xA6, 0x96, 0x97, 0x78, 0x79, 0x78, 0x69, 0x78, 0x87,   //1917  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x79, 0x79, 0x79, 0x69, 0x78, 0x77,   //1918  
	0x96, 0xA5, 0x97, 0x96, 0x97, 0x87, 0x79, 0x79, 0x69, 0x69, 0x78, 0x78,   //1919  
	0x96, 0xA5, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x79, 0x77, 0x87,   //1920  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x78, 0x79, 0x78, 0x69, 0x78, 0x87,   //1921  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x79, 0x79, 0x79, 0x69, 0x78, 0x77,   //1922  
	0x96, 0xA4, 0x96, 0x96, 0x97, 0x87, 0x79, 0x79, 0x69, 0x69, 0x78, 0x78,   //1923  
	0x96, 0xA5, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x79, 0x77, 0x87,   //1924  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x78, 0x79, 0x78, 0x69, 0x78, 0x87,   //1925  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1926  
	0x96, 0xA4, 0x96, 0x96, 0x97, 0x87, 0x79, 0x79, 0x79, 0x69, 0x78, 0x78,   //1927  
	0x96, 0xA5, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //1928  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x79, 0x77, 0x87,   //1929  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1930  
	0x96, 0xA4, 0x96, 0x96, 0x97, 0x87, 0x79, 0x79, 0x79, 0x69, 0x78, 0x78,   //1931  
	0x96, 0xA5, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //1932  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //1933  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1934  
	0x96, 0xA4, 0x96, 0x96, 0x97, 0x97, 0x79, 0x79, 0x79, 0x69, 0x78, 0x78,   //1935  
	0x96, 0xA5, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //1936  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //1937  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1938  
	0x96, 0xA4, 0x96, 0x96, 0x97, 0x97, 0x79, 0x79, 0x79, 0x69, 0x78, 0x78,   //1939  
	0x96, 0xA5, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //1940  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //1941  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1942  
	0x96, 0xA4, 0x96, 0x96, 0x97, 0x97, 0x79, 0x79, 0x79, 0x69, 0x78, 0x78,   //1943  
	0x96, 0xA5, 0x96, 0xA5, 0xA6, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //1944  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x79, 0x77, 0x87,   //1945  
	0x95, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x78, 0x69, 0x78, 0x77,   //1946  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x79, 0x79, 0x79, 0x69, 0x78, 0x78,   //1947  
	0x96, 0xA5, 0xA6, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x78, 0x78, 0x87, 0x87,   //1948  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x79, 0x78, 0x79, 0x77, 0x87,   //1949  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x78, 0x79, 0x78, 0x69, 0x78, 0x77,   //1950  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x79, 0x79, 0x79, 0x69, 0x78, 0x78,   //1951  
	0x96, 0xA5, 0xA6, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x78, 0x78, 0x87, 0x87,   //1952  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x79, 0x77, 0x87,   //1953  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x78, 0x79, 0x78, 0x68, 0x78, 0x87,   //1954  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1955  
	0x96, 0xA5, 0xA5, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x78, 0x78, 0x87, 0x87,   //1956  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x79, 0x77, 0x87,   //1957  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //1958  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1959  
	0x96, 0xA4, 0xA5, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //1960  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //1961  
	0x96, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //1962  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1963  
	0x96, 0xA4, 0xA5, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //1964  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //1965  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //1966  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1967  
	0x96, 0xA4, 0xA5, 0xA5, 0xA6, 0xA6, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //1968  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //1969  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //1970  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x79, 0x69, 0x78, 0x77,   //1971  
	0x96, 0xA4, 0xA5, 0xA5, 0xA6, 0xA6, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //1972  
	0xA5, 0xB5, 0x96, 0xA5, 0xA6, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //1973  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //1974  
	0x96, 0xB4, 0x96, 0xA6, 0x97, 0x97, 0x78, 0x79, 0x78, 0x69, 0x78, 0x77,   //1975  
	0x96, 0xA4, 0xA5, 0xB5, 0xA6, 0xA6, 0x88, 0x89, 0x88, 0x78, 0x87, 0x87,   //1976  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x88, 0x78, 0x78, 0x87, 0x87,   //1977  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x79, 0x78, 0x87,   //1978  
	0x96, 0xB4, 0x96, 0xA6, 0x96, 0x97, 0x78, 0x79, 0x78, 0x69, 0x78, 0x77,   //1979  
	0x96, 0xA4, 0xA5, 0xB5, 0xA6, 0xA6, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //1980  
	0xA5, 0xB4, 0x96, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x78, 0x78, 0x77, 0x87,   //1981  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x79, 0x77, 0x87,   //1982  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x78, 0x79, 0x78, 0x69, 0x78, 0x77,   //1983  
	0x96, 0xB4, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x88, 0x78, 0x87, 0x87,   //1984  
	0xA5, 0xB4, 0xA6, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x78, 0x78, 0x87, 0x87,   //1985  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x79, 0x77, 0x87,   //1986  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x79, 0x78, 0x69, 0x78, 0x87,   //1987  
	0x96, 0xB4, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x88, 0x78, 0x87, 0x86,   //1988  
	0xA5, 0xB4, 0xA5, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //1989  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x79, 0x77, 0x87,   //1990  
	0x95, 0xB4, 0x96, 0xA5, 0x86, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //1991  
	0x96, 0xB4, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x88, 0x78, 0x87, 0x86,   //1992  
	0xA5, 0xB3, 0xA5, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //1993  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //1994  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x76, 0x78, 0x69, 0x78, 0x87,   //1995  
	0x96, 0xB4, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x88, 0x78, 0x87, 0x86,   //1996  
	0xA5, 0xB3, 0xA5, 0xA5, 0xA6, 0xA6, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //1997  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //1998  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //1999  
	0x96, 0xB4, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x88, 0x78, 0x87, 0x86,   //2000  
	0xA5, 0xB3, 0xA5, 0xA5, 0xA6, 0xA6, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //2001  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //2002  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //2003  
	0x96, 0xB4, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x88, 0x78, 0x87, 0x86,   //2004  
	0xA5, 0xB3, 0xA5, 0xA5, 0xA6, 0xA6, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //2005  
	0xA5, 0xB4, 0x96, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x78, 0x78, 0x87, 0x87,   //2006  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x69, 0x78, 0x87,   //2007  
	0x96, 0xB4, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x87, 0x78, 0x87, 0x86,   //2008  
	0xA5, 0xB3, 0xA5, 0xB5, 0xA6, 0xA6, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //2009  
	0xA5, 0xB4, 0x96, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x78, 0x78, 0x87, 0x87,   //2010  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x79, 0x78, 0x87,   //2011  
	0x96, 0xB4, 0xA5, 0xB5, 0xA5, 0xA6, 0x87, 0x88, 0x87, 0x78, 0x87, 0x86,   //2012  
	0xA5, 0xB3, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x88, 0x78, 0x87, 0x87,   //2013  
	0xA5, 0xB4, 0x96, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x78, 0x78, 0x87, 0x87,   //2014  
	0x95, 0xB4, 0x96, 0xA5, 0x96, 0x97, 0x88, 0x78, 0x78, 0x79, 0x77, 0x87,   //2015  
	0x95, 0xB4, 0xA5, 0xB4, 0xA5, 0xA6, 0x87, 0x88, 0x87, 0x78, 0x87, 0x86,   //2016  
	0xA5, 0xC3, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x88, 0x78, 0x87, 0x87,   //2017  
	0xA5, 0xB4, 0xA6, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x78, 0x78, 0x87, 0x87,   //2018  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x79, 0x77, 0x87,   //2019  
	0x95, 0xB4, 0xA5, 0xB4, 0xA5, 0xA6, 0x97, 0x87, 0x87, 0x78, 0x87, 0x86,   //2020  
	0xA5, 0xC3, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x88, 0x78, 0x87, 0x86,   //2021  
	0xA5, 0xB4, 0xA5, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //2022  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x79, 0x77, 0x87,   //2023  
	0x95, 0xB4, 0xA5, 0xB4, 0xA5, 0xA6, 0x97, 0x87, 0x87, 0x78, 0x87, 0x96,   //2024  
	0xA5, 0xC3, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x88, 0x78, 0x87, 0x86,   //2025  
	0xA5, 0xB3, 0xA5, 0xA5, 0xA6, 0xA6, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //2026  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //2027  
	0x95, 0xB4, 0xA5, 0xB4, 0xA5, 0xA6, 0x97, 0x87, 0x87, 0x78, 0x87, 0x96,   //2028  
	0xA5, 0xC3, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x88, 0x78, 0x87, 0x86,   //2029  
	0xA5, 0xB3, 0xA5, 0xA5, 0xA6, 0xA6, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //2030  
	0xA5, 0xB4, 0x96, 0xA5, 0x96, 0x96, 0x88, 0x78, 0x78, 0x78, 0x87, 0x87,   //2031  
	0x95, 0xB4, 0xA5, 0xB4, 0xA5, 0xA6, 0x97, 0x87, 0x87, 0x78, 0x87, 0x96,   //2032  
	0xA5, 0xC3, 0xA5, 0xB5, 0xA6, 0xA6, 0x88, 0x88, 0x88, 0x78, 0x87, 0x86,   //2033  
	0xA5, 0xB3, 0xA5, 0xA5, 0xA6, 0xA6, 0x88, 0x78, 0x88, 0x78, 0x87, 0x87,   //2034  
	0xA5, 0xB4, 0x96, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x78, 0x78, 0x87, 0x87,   //2035  
	0x95, 0xB4, 0xA5, 0xB4, 0xA5, 0xA6, 0x97, 0x87, 0x87, 0x78, 0x87, 0x96,   //2036  
	0xA5, 0xC3, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x88, 0x78, 0x87, 0x86,   //2037  
	0xA5, 0xB3, 0xA5, 0xA5, 0xA6, 0xA6, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //2038  
	0xA5, 0xB4, 0x96, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x78, 0x78, 0x87, 0x87,   //2039  
	0x95, 0xB4, 0xA5, 0xB4, 0xA5, 0xA6, 0x97, 0x87, 0x87, 0x78, 0x87, 0x96,   //2040  
	0xA5, 0xC3, 0xA5, 0xB5, 0xA5, 0xA6, 0x87, 0x88, 0x87, 0x78, 0x87, 0x86,   //2041  
	0xA5, 0xB3, 0xA5, 0xB5, 0xA6, 0xA6, 0x88, 0x88, 0x88, 0x78, 0x87, 0x87,   //2042  
	0xA5, 0xB4, 0x96, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x78, 0x78, 0x87, 0x87,   //2043  
	0x95, 0xB4, 0xA5, 0xB4, 0xA5, 0xA6, 0x97, 0x87, 0x87, 0x88, 0x87, 0x96,   //2044  
	0xA5, 0xC3, 0xA5, 0xB4, 0xA5, 0xA6, 0x87, 0x88, 0x87, 0x78, 0x87, 0x86,   //2045  
	0xA5, 0xB3, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x88, 0x78, 0x87, 0x87,   //2046  
	0xA5, 0xB4, 0x96, 0xA5, 0xA6, 0x96, 0x88, 0x88, 0x78, 0x78, 0x87, 0x87,   //2047  
	0x95, 0xB4, 0xA5, 0xB4, 0xA5, 0xA5, 0x97, 0x87, 0x87, 0x88, 0x86, 0x96,   //2048  
	0xA4, 0xC3, 0xA5, 0xA5, 0xA5, 0xA6, 0x97, 0x87, 0x87, 0x78, 0x87, 0x86,   //2049  
	0xA5, 0xC3, 0xA5, 0xB5, 0xA6, 0xA6, 0x87, 0x88, 0x78, 0x78, 0x87, 0x87    //2050  
};

static const ulonglong_t __msOfDay = 24 * 60 * 60 * 1000;
static const ulonglong_t __msOfHour = 60 * 60 * 1000;
static const ulonglong_t __msOfMinute = 60 * 1000;
static const ulonglong_t __msOfSecond = 1000;


DateTime::DateTime() {
}

DateTime::~DateTime() {
}

void DateTime::initSystemTime() {
	initSystemTime(OsUtil::seconds() * 1000);
}

void DateTime::initSystemTime(uint_t start_time) {
	time_t systime;
	::time(&systime);

	struct tm *stm = ::localtime(&systime);

	m_startData.year = stm->tm_year + 1900;
	m_startData.month = stm->tm_mon + 1;
	m_startData.day = stm->tm_mday;
	m_startData.hour = stm->tm_hour;
	m_startData.minute = stm->tm_min;
	m_startData.second = stm->tm_sec;
	m_startData.milliseconds = 0;

	m_startData.totaltime = calcDateDiff(m_startData.year, m_startData.month, m_startData.day) * __msOfDay;
	m_startData.totaltime += m_startData.hour * __msOfHour;
	m_startData.totaltime += m_startData.minute * __msOfMinute;
	m_startData.totaltime += m_startData.second * __msOfSecond;

	m_startMilliseconds = start_time;

	update(0);
}

void DateTime::init(ushort_t year, ushort_t month, ushort_t day, ushort_t hour, ushort_t minute, ushort_t second, uint_t start_time) {
	m_startData.year = year;
	m_startData.month = month;
	m_startData.day = day;
	m_startData.hour = hour;
	m_startData.minute = minute;
	m_startData.second = second;
	m_startData.milliseconds = 0;

	m_startData.totaltime = calcDateDiff(m_startData.year, m_startData.month, m_startData.day) * __msOfDay;
	m_startData.totaltime += m_startData.hour * __msOfHour;
	m_startData.totaltime += m_startData.minute * __msOfMinute;
	m_startData.totaltime += m_startData.second * __msOfSecond;

	m_startMilliseconds = start_time;

	update(0);
}

void DateTime::init(ushort_t year, ushort_t month, ushort_t day, ushort_t hour, ushort_t minute, ushort_t second) {
	init(year, month, day, hour, minute, second, OsUtil::seconds()*1000);
}

void DateTime::init(ushort_t hour, ushort_t minute, ushort_t second, uint_t startTime) {
	time_t systime;
	::time(&systime);

	struct tm *stm = ::localtime(&systime);

	ushort_t year = stm->tm_year + 1900;
	ushort_t month = stm->tm_mon + 1;
	ushort_t day = stm->tm_mday;

	init(year, month, day, hour, minute, second, startTime);
}

void DateTime::init(ushort_t hour, ushort_t minute, ushort_t second) {
	init(hour, minute, second, OsUtil::seconds()*1000);
}

DateTime::Data DateTime::getData() {
	return m_curData;
}

#if 0
DateTime::Data DateTime::getData(uint_t now_time) {
	Data data;

	// get total time
	data.totaltime = m_startData.totaltime + now_time - m_startMilliseconds;

	// calculate everything from total time
	l_calcDate(data.year, data.month, data.day, data.totaltime / __msOfDay);

	ulonglong_t dayms = data.totaltime % __msOfDay;

	// calculates hours
	data.hour = dayms / __msOfHour;
	dayms = dayms % __msOfHour;

	// calculates minutes
	data.minute = dayms / __msOfMinute;
	dayms = dayms % __msOfMinute;

	// calculates seconds
	data.second = dayms / __msOfSecond;
	dayms = dayms % __msOfSecond;

	// calculates milliseconds
	data.milliseconds = dayms;

	// day of week
	data.dayOfWeek = weekDay(data.year, data.month, data.day);

	// lunar time
	data.solarTerm = getLunarDate(data.year, data.month, data.day, data.lunarYear, data.lunarMonth, data.lunarDay, data.leapMonth);

	lunarYearToTianganDizhi(data.lunarYear, data.tiangan, data.dizhi);

	// sun and moon
	data.sunAngle = float(data.totaltime % __msOfDay) / __msOfDay * 360 + 270;
	data.moonAngle = data.sunAngle - data.lunarDay / 30.f * 360 + 360;

	// normalize to 0~360
	while (data.sunAngle > 360.f)
		data.sunAngle -= 360.f;

	while (data.moonAngle > 360.f)
		data.moonAngle -= 360.f;

	return data;
}
#endif

void DateTime::update(int curtime) {
	Data &data = m_curData;

	m_curMilliseconds = curtime;

	// get total time
	data.totaltime = m_startData.totaltime + m_curMilliseconds - m_startMilliseconds;

	// calculate everything from total time
	l_calcDate(data.year, data.month, data.day, data.totaltime / __msOfDay);

	ulonglong_t dayms = data.totaltime % __msOfDay;

	// calculates hours
	data.hour = dayms / __msOfHour;
	dayms = dayms % __msOfHour;

	// calculates minutes
	data.minute = dayms / __msOfMinute;
	dayms = dayms % __msOfMinute;

	// calculates seconds
	data.second = dayms / __msOfSecond;
	dayms = dayms % __msOfSecond;

	// calculates milliseconds
	data.milliseconds = dayms;

	// day of week
	data.dayOfWeek = weekDay(data.year, data.month, data.day);

	// lunar time
	data.solarTerm = getLunarDate(data.year, data.month, data.day, data.lunarYear, data.lunarMonth, data.lunarDay, data.leapMonth);

	lunarYearToTianganDizhi(data.lunarYear, data.tiangan, data.dizhi);

	// sun and moon
	data.sunAngle = float(data.totaltime % __msOfDay) / __msOfDay * 360 + 270;
	data.moonAngle = data.sunAngle - data.lunarDay / 30.f * 360 + 360;

	// normalize to 0~360
	while (data.sunAngle > 360.f)
		data.sunAngle -= 360.f;

	while (data.moonAngle > 360.f)
		data.moonAngle -= 360.f;
}


bool DateTime::isLeapYear(ushort_t year) {
	return !(year % 4) && (year % 100) || !(year % 400);
}


ushort_t DateTime::weekDay(ushort_t year, ushort_t month, ushort_t day) {
	// month day mod 7  
	ushort_t monthday[]={ 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };

	ushort_t iDays = (year-1)%7 + (year-1)/4 - (year-1)/100 +(year-1)/400;  
	iDays += (monthday[month-1] + day);  

	// if is leap year
	if (isLeapYear(year) && month > 2)
		iDays++;

	// 0 is sunday, 1 is monday...
	return iDays % 7;  
}
 

ushort_t DateTime::monthDays(ushort_t year, ushort_t month) {
	switch (month) {  
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:  
		return 31;  
		break;  
	case 4:
	case 6:
	case 9:
	case 11:  
		return 30;  
		break;  
	case 2:  
		// if is leap year
		if (isLeapYear(year))  
			return 29;  
		else  
			return 28;  
		break;  
	}  
	return 0;  
}

inline int MakeInt32(ushort_t low, ushort_t hi) {
	return (hi << 16) + low;
}

inline ushort_t HighUint16(int tmp) {
	return tmp >> 16;
}

inline ushort_t LowUint16(int tmp) {
	return tmp & 0xFFFF;
}

int DateTime::lunarMonthDays(ushort_t lunarYear, ushort_t lunarMonth) {
	if (lunarYear < START_YEAR) 
		return 30; 

	ushort_t height =0 ,low =29; 
	int iBit = 16 - lunarMonth; 

	if (lunarMonth > getLeapMonth(lunarYear) && getLeapMonth(lunarYear))  
		iBit--;  

	if (gLunarMonthDay[lunarYear - START_YEAR] & (1<<iBit)) 
		low++; 

	if (lunarMonth == getLeapMonth(lunarYear)) 
		if (gLunarMonthDay[lunarYear - START_YEAR] & (1<< (iBit -1))) 
			height =30; 
		else 
			height =29; 

	return MakeInt32(low, height); 
}
  

ushort_t DateTime::lunarYearDays(ushort_t lunarYear) {
	ushort_t days =0; 
	for (ushort_t i=1; i<=12; i++) { 
		int tmp = lunarMonthDays(lunarYear, i); 
		days += HighUint16(tmp); 
		days += LowUint16(tmp); 
	} 
	return days; 
}

void DateTime::lunarYearToTianganDizhi(ushort_t lunarYear, ushort_t &tiangan, ushort_t &dizhi) {
	tiangan = (lunarYear - 4) % 10;
	dizhi = (lunarYear - 4) % 12;
}

ushort_t DateTime::getLeapMonth(ushort_t lunarYear) {
	byte_t &flag = gLunarMonth[(lunarYear - START_YEAR)/2];  
	return  (lunarYear - START_YEAR) % 2 ? flag & 0x0f : flag >> 4;  
}
  

std::string DateTime::formatLunarYear(ushort_t year) {
	wchar_t szText1[] = { 0x7532, 0x4e59, 0x4e19, 0x4e01, 0x620a, 0x5df1, 0x5e9a, 0x8f9b, 0x58ec, 0x7678 }; // 甲乙丙丁戊己庚辛壬癸
	wchar_t szText2[] = { 0x5b50, 0x4e11, 0x5bc5, 0x536f, 0x8fb0, 0x5df3, 0x5348, 0x672a, 0x7533, 0x9149, 0x620c, 0x4ea5}; // 子丑寅卯辰巳午未申酉戌亥

	wchar_t result[12];

	result[0] = szText1[(year-4)%10];
	result[1] = szText2[((year-4)%12)];
	result[2] = 0x5e74; // L'年'
	result[3] = 0;
	
	return w2u(result);
}
 

std::string DateTime::formatMonth(ushort_t month, bool lunar) {
	return std::string();
}
  

std::string DateTime::formatLunarDay(ushort_t  day) {
	return std::string();
}
  

int DateTime::calcDateDiff(ushort_t endYear, ushort_t endMonth, ushort_t endDay, ushort_t startYear, ushort_t startMonth, ushort_t startDay) {
	ushort_t monthday[]={ 0, 31, 59 ,90, 120, 151, 181, 212, 243, 273, 304, 334 };

	// calculate two years' Jun 1 days
	int diffDays =(endYear - startYear)*365;
	diffDays += (endYear - 1) / 4 - (startYear - 1) / 4;
	diffDays -= ((endYear - 1) / 100 - (startYear - 1) / 100);
	diffDays += (endYear - 1) / 400 - (startYear - 1) / 400;

	// add endYear from Jun 1's day to endMonth and endDay
	diffDays += monthday[endMonth-1] + (isLeapYear(endYear) && endMonth > 2 ? 1: 0);

	diffDays += endDay;

	// sub startYear from Jun 1 to startMonth and StartDay's days
	diffDays -= (monthday[startMonth-1] + (isLeapYear(startYear) && startMonth > 2 ? 1: 0));
	diffDays -= startDay;

	return diffDays;
}
  

ushort_t DateTime::getLunarDate(ushort_t year, ushort_t month, ushort_t day, ushort_t &lunarYear, ushort_t &lunarMonth, ushort_t &lunarDay, bool &leapMonth) {
	l_calcLunarDate(lunarYear, lunarMonth, lunarDay, leapMonth, calcDateDiff(year, month, day)); 

	return l_getLunarHolDay(year, month, day); 
}
  
void DateTime::l_calcLunarDate(ushort_t &year, ushort_t &month, ushort_t &day, bool &leapMonth, int span_days) {
	// 阳历1901年2月19日为阴历1901年正月初一
	// 阳历1901年1月1日到2月19日共有49天
	if (span_days < 49) {
		year = START_YEAR - 1;
		if (span_days < 19) {
			month = 11;
			day = 11 + ushort_t(span_days);
		} else {
			month = 12;
			day = ushort_t(span_days) -18;
		}
		return;
	}

	// 下面从阴历1901年正月初一算起 
	span_days -= 49;
	year = START_YEAR;
	month = 1;
	day = 1;

	// 计算年
	int tmp = lunarYearDays(year);
	while (span_days >= tmp) {
		span_days -= tmp;
		tmp = lunarYearDays(++year);
	}

	// 计算月
	leapMonth = false;
	tmp = LowUint16(lunarMonthDays(year, month));
	while (span_days >= tmp) {  
		span_days -= tmp;  
		if (month == getLeapMonth(year)) {  
			tmp  = HighUint16(lunarMonthDays(year, month));  
			if (span_days < tmp) {
				leapMonth = true;
				break; 
			}
			span_days -= tmp; 
		} 
		tmp = LowUint16(lunarMonthDays(year, ++month)); 
	} 

	// 计算日 
	day += ushort_t(span_days); 
}

void DateTime::l_calcDate(ushort_t &year, ushort_t &month ,ushort_t &day, int span_days) {
	year = START_YEAR;
	month = 1;
	day = 1;

	while (1) {
		int y_days = isLeapYear(year) ? 366 : 365;

		if (span_days < y_days)
			break;

		span_days -= y_days;
		year++;
	}

	while (1) {
		if (span_days < monthDays(year, month))
			break;

		span_days -= monthDays(year, month);
		month++;
	}

	day += span_days;
}

ushort_t DateTime::l_getLunarHolDay(ushort_t year, ushort_t month, ushort_t day) {
	byte_t &flag = gLunarHolDay[(year - START_YEAR)*12+month -1];
	ushort_t _day;

	if (day < 15) 
		_day= 15 - ((flag>>4)&0x0f);  
	else  
		_day = ((flag)&0x0f)+15;  
	if (day == _day)  
		return (month - 1) * 2 + (day > 15 ? 1 : 0) + 1;   
	else  
		return 0;  
}

AX_END_NAMESPACE
