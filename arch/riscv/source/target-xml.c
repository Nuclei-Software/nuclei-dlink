/*
 * Copyright (c) 2019 zoomdy@163.com
 * Copyright (c) 2020, Micha Hoiting <micha.hoiting@gmail.com>
 * Copyright (c) 2022 Nuclei Limited. All rights reserved.
 *
 * \file  rv-link/main.c
 * \brief Main RV-Link application.
 *
 * RV-LINK is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v1 for more details.
 */

#include "config.h"
#include "riscv-target.h"

uint32_t rv_target_xml_len = 0;

static const char target_xml_xlen4_flen4[] =
{
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target version=\"1.0\">"
    "<architecture>riscv:rv32</architecture>"
    "<feature name=\"org.gnu.gdb.riscv.cpu\">"
    "<reg name=\"zero\" bitsize=\"32\" regnum=\"0\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"ra\" bitsize=\"32\" regnum=\"1\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"sp\" bitsize=\"32\" regnum=\"2\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"gp\" bitsize=\"32\" regnum=\"3\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"tp\" bitsize=\"32\" regnum=\"4\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t0\" bitsize=\"32\" regnum=\"5\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t1\" bitsize=\"32\" regnum=\"6\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t2\" bitsize=\"32\" regnum=\"7\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"fp\" bitsize=\"32\" regnum=\"8\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s1\" bitsize=\"32\" regnum=\"9\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a0\" bitsize=\"32\" regnum=\"10\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a1\" bitsize=\"32\" regnum=\"11\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a2\" bitsize=\"32\" regnum=\"12\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a3\" bitsize=\"32\" regnum=\"13\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a4\" bitsize=\"32\" regnum=\"14\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a5\" bitsize=\"32\" regnum=\"15\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a6\" bitsize=\"32\" regnum=\"16\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a7\" bitsize=\"32\" regnum=\"17\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s2\" bitsize=\"32\" regnum=\"18\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s3\" bitsize=\"32\" regnum=\"19\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s4\" bitsize=\"32\" regnum=\"20\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s5\" bitsize=\"32\" regnum=\"21\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s6\" bitsize=\"32\" regnum=\"22\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s7\" bitsize=\"32\" regnum=\"23\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s8\" bitsize=\"32\" regnum=\"24\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s9\" bitsize=\"32\" regnum=\"25\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s10\" bitsize=\"32\" regnum=\"26\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s11\" bitsize=\"32\" regnum=\"27\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t3\" bitsize=\"32\" regnum=\"28\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t4\" bitsize=\"32\" regnum=\"29\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t5\" bitsize=\"32\" regnum=\"30\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t6\" bitsize=\"32\" regnum=\"31\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"pc\" bitsize=\"32\" regnum=\"32\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "</feature>"
    "<feature name=\"org.gnu.gdb.riscv.fpu\">"
    "<reg name=\"ft0\"  bitsize=\"32\" regnum=\"33\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft1\"  bitsize=\"32\" regnum=\"34\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft2\"  bitsize=\"32\" regnum=\"35\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft3\"  bitsize=\"32\" regnum=\"36\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft4\"  bitsize=\"32\" regnum=\"37\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft5\"  bitsize=\"32\" regnum=\"38\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft6\"  bitsize=\"32\" regnum=\"39\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft7\"  bitsize=\"32\" regnum=\"40\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs0\"  bitsize=\"32\" regnum=\"41\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs1\"  bitsize=\"32\" regnum=\"42\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa0\"  bitsize=\"32\" regnum=\"43\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa1\"  bitsize=\"32\" regnum=\"44\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa2\"  bitsize=\"32\" regnum=\"45\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa3\"  bitsize=\"32\" regnum=\"46\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa4\"  bitsize=\"32\" regnum=\"47\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa5\"  bitsize=\"32\" regnum=\"48\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa6\"  bitsize=\"32\" regnum=\"49\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa7\"  bitsize=\"32\" regnum=\"50\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs2\"  bitsize=\"32\" regnum=\"51\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs3\"  bitsize=\"32\" regnum=\"52\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs4\"  bitsize=\"32\" regnum=\"53\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs5\"  bitsize=\"32\" regnum=\"54\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs6\"  bitsize=\"32\" regnum=\"55\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs7\"  bitsize=\"32\" regnum=\"56\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs8\"  bitsize=\"32\" regnum=\"57\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs9\"  bitsize=\"32\" regnum=\"58\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs10\" bitsize=\"32\" regnum=\"59\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs11\" bitsize=\"32\" regnum=\"60\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft8\"  bitsize=\"32\" regnum=\"61\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft9\"  bitsize=\"32\" regnum=\"62\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft10\" bitsize=\"32\" regnum=\"63\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft11\" bitsize=\"32\" regnum=\"64\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "</feature>"
    "</target>"
};

static const char target_xml_xlen4_flen8[] =
{
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target version=\"1.0\">"
    "<architecture>riscv:rv32</architecture>"
    "<feature name=\"org.gnu.gdb.riscv.cpu\">"
    "<reg name=\"zero\" bitsize=\"32\" regnum=\"0\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"ra\" bitsize=\"32\" regnum=\"1\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"sp\" bitsize=\"32\" regnum=\"2\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"gp\" bitsize=\"32\" regnum=\"3\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"tp\" bitsize=\"32\" regnum=\"4\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t0\" bitsize=\"32\" regnum=\"5\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t1\" bitsize=\"32\" regnum=\"6\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t2\" bitsize=\"32\" regnum=\"7\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"fp\" bitsize=\"32\" regnum=\"8\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s1\" bitsize=\"32\" regnum=\"9\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a0\" bitsize=\"32\" regnum=\"10\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a1\" bitsize=\"32\" regnum=\"11\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a2\" bitsize=\"32\" regnum=\"12\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a3\" bitsize=\"32\" regnum=\"13\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a4\" bitsize=\"32\" regnum=\"14\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a5\" bitsize=\"32\" regnum=\"15\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a6\" bitsize=\"32\" regnum=\"16\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a7\" bitsize=\"32\" regnum=\"17\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s2\" bitsize=\"32\" regnum=\"18\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s3\" bitsize=\"32\" regnum=\"19\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s4\" bitsize=\"32\" regnum=\"20\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s5\" bitsize=\"32\" regnum=\"21\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s6\" bitsize=\"32\" regnum=\"22\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s7\" bitsize=\"32\" regnum=\"23\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s8\" bitsize=\"32\" regnum=\"24\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s9\" bitsize=\"32\" regnum=\"25\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s10\" bitsize=\"32\" regnum=\"26\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s11\" bitsize=\"32\" regnum=\"27\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t3\" bitsize=\"32\" regnum=\"28\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t4\" bitsize=\"32\" regnum=\"29\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t5\" bitsize=\"32\" regnum=\"30\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t6\" bitsize=\"32\" regnum=\"31\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"pc\" bitsize=\"32\" regnum=\"32\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "</feature>"
    "<feature name=\"org.gnu.gdb.riscv.fpu\">"
    "<reg name=\"ft0\"  bitsize=\"64\" regnum=\"33\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft1\"  bitsize=\"64\" regnum=\"34\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft2\"  bitsize=\"64\" regnum=\"35\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft3\"  bitsize=\"64\" regnum=\"36\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft4\"  bitsize=\"64\" regnum=\"37\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft5\"  bitsize=\"64\" regnum=\"38\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft6\"  bitsize=\"64\" regnum=\"39\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft7\"  bitsize=\"64\" regnum=\"40\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs0\"  bitsize=\"64\" regnum=\"41\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs1\"  bitsize=\"64\" regnum=\"42\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa0\"  bitsize=\"64\" regnum=\"43\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa1\"  bitsize=\"64\" regnum=\"44\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa2\"  bitsize=\"64\" regnum=\"45\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa3\"  bitsize=\"64\" regnum=\"46\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa4\"  bitsize=\"64\" regnum=\"47\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa5\"  bitsize=\"64\" regnum=\"48\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa6\"  bitsize=\"64\" regnum=\"49\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa7\"  bitsize=\"64\" regnum=\"50\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs2\"  bitsize=\"64\" regnum=\"51\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs3\"  bitsize=\"64\" regnum=\"52\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs4\"  bitsize=\"64\" regnum=\"53\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs5\"  bitsize=\"64\" regnum=\"54\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs6\"  bitsize=\"64\" regnum=\"55\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs7\"  bitsize=\"64\" regnum=\"56\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs8\"  bitsize=\"64\" regnum=\"57\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs9\"  bitsize=\"64\" regnum=\"58\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs10\" bitsize=\"64\" regnum=\"59\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs11\" bitsize=\"64\" regnum=\"60\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft8\"  bitsize=\"64\" regnum=\"61\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft9\"  bitsize=\"64\" regnum=\"62\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft10\" bitsize=\"64\" regnum=\"63\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft11\" bitsize=\"64\" regnum=\"64\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "</feature>"
    "</target>"
};

static const char target_xml_xlen8_flen4[] =
{
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target version=\"1.0\">"
    "<architecture>riscv:rv64</architecture>"
    "<feature name=\"org.gnu.gdb.riscv.cpu\">"
    "<reg name=\"zero\" bitsize=\"64\" regnum=\"0\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"ra\"   bitsize=\"64\" regnum=\"1\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"sp\"   bitsize=\"64\" regnum=\"2\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"gp\"   bitsize=\"64\" regnum=\"3\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"tp\"   bitsize=\"64\" regnum=\"4\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t0\"   bitsize=\"64\" regnum=\"5\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t1\"   bitsize=\"64\" regnum=\"6\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t2\"   bitsize=\"64\" regnum=\"7\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"fp\"   bitsize=\"64\" regnum=\"8\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s1\"   bitsize=\"64\" regnum=\"9\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a0\"   bitsize=\"64\" regnum=\"10\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a1\"   bitsize=\"64\" regnum=\"11\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a2\"   bitsize=\"64\" regnum=\"12\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a3\"   bitsize=\"64\" regnum=\"13\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a4\"   bitsize=\"64\" regnum=\"14\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a5\"   bitsize=\"64\" regnum=\"15\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a6\"   bitsize=\"64\" regnum=\"16\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a7\"   bitsize=\"64\" regnum=\"17\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s2\"   bitsize=\"64\" regnum=\"18\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s3\"   bitsize=\"64\" regnum=\"19\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s4\"   bitsize=\"64\" regnum=\"20\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s5\"   bitsize=\"64\" regnum=\"21\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s6\"   bitsize=\"64\" regnum=\"22\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s7\"   bitsize=\"64\" regnum=\"23\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s8\"   bitsize=\"64\" regnum=\"24\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s9\"   bitsize=\"64\" regnum=\"25\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s10\"  bitsize=\"64\" regnum=\"26\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s11\"  bitsize=\"64\" regnum=\"27\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t3\"   bitsize=\"64\" regnum=\"28\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t4\"   bitsize=\"64\" regnum=\"29\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t5\"   bitsize=\"64\" regnum=\"30\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t6\"   bitsize=\"64\" regnum=\"31\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"pc\"   bitsize=\"64\" regnum=\"32\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "</feature>"
    "<feature name=\"org.gnu.gdb.riscv.fpu\">"
    "<reg name=\"ft0\"  bitsize=\"32\" regnum=\"33\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft1\"  bitsize=\"32\" regnum=\"34\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft2\"  bitsize=\"32\" regnum=\"35\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft3\"  bitsize=\"32\" regnum=\"36\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft4\"  bitsize=\"32\" regnum=\"37\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft5\"  bitsize=\"32\" regnum=\"38\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft6\"  bitsize=\"32\" regnum=\"39\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft7\"  bitsize=\"32\" regnum=\"40\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs0\"  bitsize=\"32\" regnum=\"41\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs1\"  bitsize=\"32\" regnum=\"42\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa0\"  bitsize=\"32\" regnum=\"43\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa1\"  bitsize=\"32\" regnum=\"44\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa2\"  bitsize=\"32\" regnum=\"45\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa3\"  bitsize=\"32\" regnum=\"46\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa4\"  bitsize=\"32\" regnum=\"47\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa5\"  bitsize=\"32\" regnum=\"48\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa6\"  bitsize=\"32\" regnum=\"49\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa7\"  bitsize=\"32\" regnum=\"50\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs2\"  bitsize=\"32\" regnum=\"51\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs3\"  bitsize=\"32\" regnum=\"52\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs4\"  bitsize=\"32\" regnum=\"53\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs5\"  bitsize=\"32\" regnum=\"54\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs6\"  bitsize=\"32\" regnum=\"55\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs7\"  bitsize=\"32\" regnum=\"56\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs8\"  bitsize=\"32\" regnum=\"57\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs9\"  bitsize=\"32\" regnum=\"58\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs10\" bitsize=\"32\" regnum=\"59\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs11\" bitsize=\"32\" regnum=\"60\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft8\"  bitsize=\"32\" regnum=\"61\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft9\"  bitsize=\"32\" regnum=\"62\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft10\" bitsize=\"32\" regnum=\"63\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft11\" bitsize=\"32\" regnum=\"64\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "</feature>"
    "</target>"
};

static const char target_xml_xlen8_flen8[] =
{
    "<?xml version=\"1.0\"?>"
    "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target version=\"1.0\">"
    "<architecture>riscv:rv64</architecture>"
    "<feature name=\"org.gnu.gdb.riscv.cpu\">"
    "<reg name=\"zero\" bitsize=\"64\" regnum=\"0\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"ra\"   bitsize=\"64\" regnum=\"1\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"sp\"   bitsize=\"64\" regnum=\"2\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"gp\"   bitsize=\"64\" regnum=\"3\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"tp\"   bitsize=\"64\" regnum=\"4\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t0\"   bitsize=\"64\" regnum=\"5\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t1\"   bitsize=\"64\" regnum=\"6\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t2\"   bitsize=\"64\" regnum=\"7\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"fp\"   bitsize=\"64\" regnum=\"8\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s1\"   bitsize=\"64\" regnum=\"9\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a0\"   bitsize=\"64\" regnum=\"10\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a1\"   bitsize=\"64\" regnum=\"11\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a2\"   bitsize=\"64\" regnum=\"12\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a3\"   bitsize=\"64\" regnum=\"13\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a4\"   bitsize=\"64\" regnum=\"14\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a5\"   bitsize=\"64\" regnum=\"15\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a6\"   bitsize=\"64\" regnum=\"16\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"a7\"   bitsize=\"64\" regnum=\"17\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s2\"   bitsize=\"64\" regnum=\"18\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s3\"   bitsize=\"64\" regnum=\"19\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s4\"   bitsize=\"64\" regnum=\"20\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s5\"   bitsize=\"64\" regnum=\"21\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s6\"   bitsize=\"64\" regnum=\"22\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s7\"   bitsize=\"64\" regnum=\"23\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s8\"   bitsize=\"64\" regnum=\"24\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s9\"   bitsize=\"64\" regnum=\"25\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s10\"  bitsize=\"64\" regnum=\"26\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"s11\"  bitsize=\"64\" regnum=\"27\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t3\"   bitsize=\"64\" regnum=\"28\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t4\"   bitsize=\"64\" regnum=\"29\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t5\"   bitsize=\"64\" regnum=\"30\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"t6\"   bitsize=\"64\" regnum=\"31\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "<reg name=\"pc\"   bitsize=\"64\" regnum=\"32\" save-restore=\"yes\" type=\"int\" group=\"general\"/>"
    "</feature>"
    "<feature name=\"org.gnu.gdb.riscv.fpu\">"
    "<reg name=\"ft0\"  bitsize=\"64\" regnum=\"33\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft1\"  bitsize=\"64\" regnum=\"34\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft2\"  bitsize=\"64\" regnum=\"35\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft3\"  bitsize=\"64\" regnum=\"36\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft4\"  bitsize=\"64\" regnum=\"37\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft5\"  bitsize=\"64\" regnum=\"38\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft6\"  bitsize=\"64\" regnum=\"39\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft7\"  bitsize=\"64\" regnum=\"40\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs0\"  bitsize=\"64\" regnum=\"41\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs1\"  bitsize=\"64\" regnum=\"42\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa0\"  bitsize=\"64\" regnum=\"43\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa1\"  bitsize=\"64\" regnum=\"44\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa2\"  bitsize=\"64\" regnum=\"45\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa3\"  bitsize=\"64\" regnum=\"46\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa4\"  bitsize=\"64\" regnum=\"47\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa5\"  bitsize=\"64\" regnum=\"48\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa6\"  bitsize=\"64\" regnum=\"49\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fa7\"  bitsize=\"64\" regnum=\"50\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs2\"  bitsize=\"64\" regnum=\"51\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs3\"  bitsize=\"64\" regnum=\"52\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs4\"  bitsize=\"64\" regnum=\"53\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs5\"  bitsize=\"64\" regnum=\"54\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs6\"  bitsize=\"64\" regnum=\"55\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs7\"  bitsize=\"64\" regnum=\"56\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs8\"  bitsize=\"64\" regnum=\"57\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs9\"  bitsize=\"64\" regnum=\"58\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs10\" bitsize=\"64\" regnum=\"59\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"fs11\" bitsize=\"64\" regnum=\"60\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft8\"  bitsize=\"64\" regnum=\"61\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft9\"  bitsize=\"64\" regnum=\"62\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft10\" bitsize=\"64\" regnum=\"63\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "<reg name=\"ft11\" bitsize=\"64\" regnum=\"64\" save-restore=\"yes\" type=\"int\" group=\"float\"/>"
    "</feature>"
    "</target>"
};

const char* rv_target_get_target_xml(void)
{
    if (XLEN_RV32 == rv_target_xlen()) {
        if (FLEN_SINGLE == rv_target_flen()) {
            return target_xml_xlen4_flen4;
        } else if (FLEN_DOUBLE == rv_target_flen()) {
            return target_xml_xlen4_flen8;
        }
    } else if (XLEN_RV64 == rv_target_xlen()) {
        if (FLEN_SINGLE == rv_target_flen()) {
            return target_xml_xlen8_flen4;
        } else if (FLEN_DOUBLE == rv_target_flen()) {
            return target_xml_xlen8_flen8;
        }
    }
}

uint32_t rv_target_get_target_xml_len(void)
{
    if (0 == rv_target_xml_len) {
        if (XLEN_RV32 == rv_target_xlen()) {
            if (FLEN_SINGLE == rv_target_flen()) {
                rv_target_xml_len = strlen(target_xml_xlen4_flen4);
            } else if (FLEN_DOUBLE == rv_target_flen()) {
                rv_target_xml_len = strlen(target_xml_xlen4_flen8);
            }
        } else if (XLEN_RV64 == rv_target_xlen()) {
            if (FLEN_SINGLE == rv_target_flen()) {
                rv_target_xml_len = strlen(target_xml_xlen8_flen4);
            } else if (FLEN_DOUBLE == rv_target_flen()) {
                rv_target_xml_len = strlen(target_xml_xlen8_flen8);
            }
        }
    }

    return rv_target_xml_len;
}
