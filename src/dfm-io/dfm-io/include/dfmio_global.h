// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFMIO_GLOBAL_H
#define DFMIO_GLOBAL_H

//  namespace
#define DFMIO dfmio
#define BEGIN_IO_NAMESPACE namespace DFMIO {
#define USING_IO_NAMESPACE using namespace DFMIO;
#define END_IO_NAMESPACE }

// fake virtual and override
// remind developers that they can override member functions
#define DFM_VIRTUAL
#define DFM_OVERRIDE

#endif // DFMIO_GLOBAL_H
