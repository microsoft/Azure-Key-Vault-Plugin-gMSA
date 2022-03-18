//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
//

#pragma once

#ifndef _VERSION_H_
#define _VERSION_H_

#define STR(value) #value
#define STRINGIZE(value) STR(value)

#define CCGAKVP_MAJORNUMBER          1
#define CCGAKVP_MINORNUMBER          1
#define CCGAKVP_BUILDNUMBER          4
#define CCGAKVP_FULLVERSION_STR \
        STRINGIZE(CCGAKVP_MAJORNUMBER) "." \
        STRINGIZE(CCGAKVP_MINORNUMBER) "." \
        STRINGIZE(CCGAKVP_BUILDNUMBER)

#endif
