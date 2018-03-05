/*
 * Copyright (C) 2009-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*
 * Regression check for buggy PKCS#12 string to key problem reported
 * in <http://thread.gmane.org/gmane.network.gnutls.general/1663>.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#define X_9607 \
  "-----BEGIN ENCRYPTED PRIVATE KEY-----\n" \
  "MIICojAcBgoqhkiG9w0BDAEDMA4ECL9rjpW835n6AgIIAASCAoAjs558e/tWq5ho\n" \
  "X3uYORURfasssTfqyZoSaTmEWJGbW7T+QK+ebZ8CyMVbR1ORD3rd6r7cWLsX3Ju0\n" \
  "hGncPFVpwCtwApZKnWCunj4KcsRuWdm1vAauRV2CDkykMzNlsJzAw+BPFKi2B7HL\n" \
  "xn5JymtqrGZF6zRDWW1x1WD3HYlq4FoNuSmNFu4fV0EyalIopIyNmZAY40lQ/FTM\n" \
  "LkTsnH2brIYHV1Bnzd/lXpXLli29OE/4WsPBTvhJLZGbJXp8ExwGuxfDnTFCPS9G\n" \
  "9uOjaBgerl2zjsdPNXBfn8hDNrs7MDqR9aC6rZR0yE1maEPv0YnnzDGRYZl6+j2K\n" \
  "FfWDMGET6SSimYCcZJwr0/xZAdw5e323k1xniCNVfbQhCQ09Cl6XoDI8IK23O8g+\n" \
  "R9o8gCikl98fJlpKjHaKfnscSE0hMzOjyAbYjFxWAlzjffzR5o+P6955dhREpCWy\n" \
  "kL2EOL2VmYfzGG4J62p9U88MXhCLuFOuHL/wtGzXwGnyqZyeZ5p2fYloGPEMVsX7\n" \
  "zHupLUpVZFe4kOBGI/IPWbc2iQTvzDtx9Jvxo5vWmyEwL8C7P/f9+zsIaXiM3Onz\n" \
  "F5qwQfCojesuelGPAfXJxJRLaHicva90+IyRFBSMKxgt3EdHER/R7huA//jzzQp9\n" \
  "eItmiv2UwAafeiPEDT74n6yBCTMPc++cJsMWL0SNIX4jYep55bgzbgGB8t/nQ0Ho\n" \
  "7/1KF1sAO3klAkrcTwL4pX2vLMa//W/H/AAQ2FL/Q+CAP7K5X2rlZxdkFlMuL3Dr\n" \
  "I0UqiStjznkoOeWjj6YT3jOvKGLWHPXqxTkW9Ln4fDvAoI9eq6UWHjf7gLYXxe/q\n" \
  "tTpNnYdy\n" \
  "-----END ENCRYPTED PRIVATE KEY-----\n"

#define X_9671 \
  "-----BEGIN ENCRYPTED PRIVATE KEY-----\n" \
  "MIICojAcBgoqhkiG9w0BDAEDMA4ECA7RZbNgWxdHAgIIAASCAoAq1B5klspIe7B/\n" \
  "R1pKifO1/29OsAQn9blIbaJ9fg62ivA3QGL0uApZ6eNFz6JEZyiRITJYhgLaWwov\n" \
  "mqKT9NiQ6iiemgxWLSSdvEXVOMRZB17F9PncpEiIBpnrisdD7h9MpS63LuJdEtiK\n" \
  "jpPwFwV3orFJceurq/R3ql2aKYc9MZSzkKd71QImgHYWv+IPCctl40/PZV08yKMn\n" \
  "RCMVFb/YYUrzaWSerroyjz4Kr8V0nEyKpk4YLv7o7WPGn4x8X30z0BRCA9CBwzHY\n" \
  "JMxu1FhOGXr6nx1XeaoCOt9JV8GWb+VzkATABPzFG915ULz0ma1petQyb18QyBsl\n" \
  "K9NZETrGzDYiNxkjqILhY6IRneB97C4kCH55qhXHFk5fjiWndpQ6+BFKqlCqm6Up\n" \
  "d1EF3uuKN+vY6xQbGCgFE4FHL46nb2YaoaqhPp4dj4qnRSllgBvmZbGTd243lAbT\n" \
  "J4dh/gzRwQYdIwbvcNVi9GGSOy/fezAwwXu3ZD9BqqqoCQJajrILuovbcPThy71k\n" \
  "H5EaegQ1rB+0/sn91JUb6w4pwN/54gzZGaz2F0/2xB9u57+PIMC9R8dU7uW/xWfA\n" \
  "WN7YTzPDNfevbx/LIa6VR5gsiRqCnthSsGvWFquRatMv1JrDfFUywFU9zk9W+iA2\n" \
  "rtNpXV140+/BDfHbYYrUIaklJsNP0FRXKpPw9wPHHmbOjHfFK+o8PrtOp3HUsCJm\n" \
  "2VpQtbNl66+rPLZLsbXhuJ5eY/BpRvrj6rDFPs19OAvYyrIsuQY8IdbZyGSKsq4u\n" \
  "UBsHZgPBh718EtWFFrsTNxMlRKoh5MwUSqkLXeDduAFG4N7nhQpDHQ5/KRPrYOMK\n" \
  "ixB1lLUK\n" \
  "-----END ENCRYPTED PRIVATE KEY-----\n"

#define X_9925 \
  "-----BEGIN ENCRYPTED PRIVATE KEY-----\n" \
  "MIICojAcBgoqhkiG9w0BDAEDMA4ECDnNkmSKl37mAgIIAASCAoAwttidBRLnnjti\n" \
  "b5BEsc8cO2vzImhJbYCrVDjkTpmS6IYD4FsC8KFDdQJrEYIptrwXn4uDWDUu6bxB\n" \
  "pb02Pj70gZiWBDU+ki1kIbsNc67rNpJfUlIU+po3UovSmrazqcHoW2IftvZo9hDF\n" \
  "FWVjc0D2fSWeaNwS7dimWxoLy1udof6n0c8UxvfnOgfSLg3qwWzc0+iMrbkvRFX5\n" \
  "9+vDCnetQ7ythKldnC5xQxShxaNF4O26D0VXdR9VYbQLslSHAzQi2wJ7Hh1fi62J\n" \
  "VUHvRNOcwhSadwNfQEtvIWoi6LfsUadvvhFAAbeSfQpSfD4iXgfcr3U2WIvjtOcL\n" \
  "cZg9HqRhGzgEuC7FLoov1re7xq3uifw+04qu8i9/fk7hUrldZCrCSKTc6GqsiY8x\n" \
  "JGOcNUgklzy6kbgIWp9O2C5Bxp1WmfnbNSMM9Z9UFTdbEa4Kz7SYd+1a8j1OWlq1\n" \
  "93AcEpD0+fpKuEs+S1RF7RRAs/Ais0VcOmgye0TLvKkhockxl4KT0SbOTeKnmxJ3\n" \
  "RSnPcHUb62EZuhHqpoHi+zjHH56sVy3RhcYsDKIh1Xh7JPGTysflOIno7ABK8Tu7\n" \
  "IcnAOCoBVTjXC5eSSeC3irvZSILHC1tBG8r1C1aSLFmxpOTCqRUwhtbw/FSqEngl\n" \
  "5pvwTz4gquyjCPjIAWlCscAbeqpBxNsmnJ0AGlaesd9/uxrWUScTnAIc+NUB9o8w\n" \
  "i+zXbOqhbKxWGfrQAo+qZtAchQ6EGxXuIxnSRlAEZtsrJt6/FXJaOIb5MvcXA/sQ\n" \
  "O2p1r9W2OZM8Jco2ftALygUFPDiIuELaiTQ8HE1heUZWy+M9gXV6wCGhIVtRYyCg\n" \
  "SSQ62gp7\n" \
  "-----END ENCRYPTED PRIVATE KEY-----\n"

#define X_9926 \
  "-----BEGIN ENCRYPTED PRIVATE KEY-----\n" \
  "MIICojAcBgoqhkiG9w0BDAEDMA4ECE8YpbN3dz05AgIIAASCAoC1wuyUEZs/FSTB\n" \
  "llt567hf1L+wiQ24L49ZvLutwb0nkilLHNXUo95mpLfzjnr7ZBbsIPV0RTdxjIKX\n" \
  "IdRD9SzMxeMUJ82obmgE2tTeOi7PqONX838Lmj3ocUR+aFBFTR1V7G2gMpQEapPX\n" \
  "gjv3kgwG5DCSj15NG8ybT4ZHWURyc/57dn0JWXc9/XUbm/+lvwwsuu9YvQ5Z76jE\n" \
  "ufiS8OCHNo1nPMCsUIw6herr2OfC5pj2H1/6bC7L/NPZJ7OM/IQoQOcNxiwx8rBS\n" \
  "zChy7dvPbJYmd5N+066mZiyFGxQwjPziXmqJztnB34P0Yp9dsiE1M+fo//f+QkFW\n" \
  "3HDMJmb+becnUAjiWuQCT/YqNjC4iHn35Jb2COPsV5KPsSaQ+6IaN4vWx7ifvHGD\n" \
  "KzkFcKQ1Be1EiOnUGBqhW4r7ASFKMtqGlTRBoc8PVMdFIpadejGW31Csz5gussa2\n" \
  "OcOLO8kULsT9QsuWyayG4SuTweClCaJ/nGJ/nDnocVPbucqRQBFn9ZRQ0VSLhDLe\n" \
  "B3HYRx3sJ9U+Xay9cgR09hMQ2ZaR/NxYlRshKEt+iiYOS42eMyMXVKfBwQwxl9Lf\n" \
  "ESBz7GF2nOT5VSSgJlAf3nbfhUABgq2zzoybKlFVpnq49Z79rB4b+lkP8jIhV5GA\n" \
  "/aUXssvs68FsqbG+T1nBnFWkJL49XENOrwDApzGllVbtaruoIe9t+qBF6rXVSjWQ\n" \
  "ZATZaSD3gOaM4Oyv+lso4GuONXkaXQRdpBmPLChdLMkcopQOQZtlKU2+rzi4Nm4X\n" \
  "lAAsR4sXmIGYJ3EgQrTDE+igMNr8o2qHIh81zqP7nWtkfTEfFqud6zoGK5aiZ4ma\n" \
  "0StcnRpp\n" \
  "-----END ENCRYPTED PRIVATE KEY-----\n"

#define X_9927 \
  "-----BEGIN ENCRYPTED PRIVATE KEY-----\n" \
  "MIICojAcBgoqhkiG9w0BDAEDMA4ECC6HV5s66uQrAgIIAASCAoAgQMR7E4EoMQSq\n" \
  "kFslHKebFtjtrCqEPW5lADxpJg8+FNOT6GCCnu8yslrmMa4l/MIs8jfkoKhP9O8W\n" \
  "IjQpwG5IGr0ZyfxYPZFTatrQ7+MvtMoQMBTxVt20oW4kT3tTF4KDf0BUsB9JCoET\n" \
  "DehlFSPTjDJav8fGbdEMhfbY6+6iBodnW7a3Ibil+7CQGeRIGDO7mEu5rBbI1fJb\n" \
  "tGEHkCd6Gvv20r/EIi6Fol9Fwc5eKxgFioIuZo3Tmqrr/9g09sv+qwkzoNFmpqby\n" \
  "AqCbgOOsckc3AXm4xZ7AX7zNSFXbfhiX1EyVvhwfJ6jiqHr32K8o5I4Cb/lzpB+q\n" \
  "WPMU/rF5bsTj0+/eySx8zkIUF/Jst9E+XtzlTFtMVzNpFYfzg3E+0qnT8KJtZJGr\n" \
  "Azz9aCNidjkjRVHUubrZ5qbjrv1eAYnFkgyw+UTyIJBeec6CRH5zob22ZMb5jKFz\n" \
  "d9reY1LZ38cQIoKThPdv9vKRVEd1I7T5MKv656+QegfqA7Kefwa0uK+TvvqBLTd1\n" \
  "mxgtkDvrID3PLZK9tVsOLMJcY1PFCNHB6T2EghMVEmMnROVLCqIN+MeraLhHemUe\n" \
  "rf6HFlOcYPV+5V8gI/DM2Fw/V+YgCzv380Z6HouZ4K1nwvEf53renettQmKxK/Fd\n" \
  "X74KqRSs6FtANdVUziGkrvNfssRjjLHxD08VfLAcpijRfNslxDIXQIASWqn3TPFY\n" \
  "uDs32vonOVrj2Zy8fIBRmENmGe5b/jnp055NLo6MWCFR3hmmeFBuXk6o1K6io3Le\n" \
  "oaeWr7BJFIxXZZ8zNUlBLGZinY50oM09DFOpiAUTQtkm8NuAThLcqmWvbw8LWmL4\n" \
  "ed6Pdtej\n" \
  "-----END ENCRYPTED PRIVATE KEY-----\n"

#define X_9928 \
  "-----BEGIN ENCRYPTED PRIVATE KEY-----\n" \
  "MIICojAcBgoqhkiG9w0BDAEDMA4ECC1OO648bIPcAgIIAASCAoDiQoIuNdleFu2V\n" \
  "I8MUwZ6I0Om2+2yHSrk7Jxd0mbIYnT832dVsWg53SkcBYggnN1bByej0qtf2pdBx\n" \
  "EKsOjU9T6XmOZyFjJKX6MK6syqFYI4Y67OzdiDS8FVMCYX8NhhsYlE1aqvBjvnjq\n" \
  "tgpR0pJg8uJ3FmUu1N/6ayjGtI9JbZFt+BkqbZxIfdaZhlXx1vgU2MtuxDultlJu\n" \
  "rjvzcCGG0z0GcVEmXUwVccvLqwnL6UnYkVAmhCzj4UvxYsMt6Dp8FPSQi54jmZKx\n" \
  "4LAOGGGZcKoOTJYCrUkW2RAV/GzbhT1kOJR2/Pw21Yw/WkVKyNE8LHghu6xr3pXy\n" \
  "MPmCn0fE751Vjefb6NOYIjvmMexaZVzBCZ6kuxEQBlGDi15lohnpZLcFilS7l5IY\n" \
  "nWZJ9qPX19O0RG9NgQ4xpxoPBdrxqP5HuieKgvAZ7RXDXeKlW/4z/Fo2dBjPc0YJ\n" \
  "Y5QPOK+i2Zux9VtMbxkXBeO7KsiosNQthFP+HitlIs72MHUsBZucEnZ9ny0S+blG\n" \
  "gKYK9xuuAPGscqaI6fcicFOc0ZmphMn5YP6D0nN9esqo44s9JX7SyLRPuHW+dH0/\n" \
  "Bdg9LikS8ROBs3Yuy9ksGHMbMsguum3mOwiY8f2NXQwVs3b7VfkIDMbYAjMGcriE\n" \
  "CsW1Z4EzQP2qCFVJYz6S3xSsKtgg3QeWKCtvGRJDbzCnaQGCrrHzyBlGZzr5NJkr\n" \
  "4x7MxbWppvVTMySJ+Y3V2DR+Q1nW5P7qzWaY9tE9d8unCym5C/S2CE/39jQ9zMmL\n" \
  "56qvh2swSrCEKInhQyqV+4msSYVElrQY0DGbg/N6TsKvN37zCqKKBIxhyb/5b2Kv\n" \
  "QvN7D2Ch\n" \
  "-----END ENCRYPTED PRIVATE KEY-----\n"

#define X_9929 \
  "-----BEGIN ENCRYPTED PRIVATE KEY-----\n" \
  "MIICojAcBgoqhkiG9w0BDAEDMA4ECAPza28YOfuMAgIIAASCAoBg+t7v3fo4gOZX\n" \
  "+/IY3xln+5pVj6LKXXgHWydK25TLD3oxlrecVKmnWWZuQIcPVosItr+KfwRMfkY5\n" \
  "BKUQZyu02ZO/u9cXe3XsmZLpiWAXVCaRfHhXkZ24PxQGIVikDc8KyHEAhX/P+e9m\n" \
  "jJEneTP+hdQvZmJGKKqOG95HkqlnH5KJhM8W7BjDgPBeCjaBcc9AzCWX+WdY4Nbn\n" \
  "LONjhe0nXPuVArLayru67q62LUf/NZOM6j7gbYe0ki94rXddabpOIGBhf9qP1pWc\n" \
  "m5RBntEOtlbuosUYhlOpse91SBM2nHnOzM1fIxX6J9p/AlctvtB+Zoqx4OEwbRxT\n" \
  "hNpCUo+3rwmAAOz6CntGHpmfFKrzc0r37aoSjnlQJKTxDRJHN43+eqbdtNpaQfDH\n" \
  "0pS4o84oO3/CgnJ45Bx3HJXNlg3YvKhHWav8wtHX085URoc8h/OJ3PiKBi7+5AYR\n" \
  "CLAaJjtTC0ReaOXjyGfhzzuux7UDl+MW0D69vaz2t7HSR2tQ4tYnA4fciqirSKdL\n" \
  "wFgewXRNxNkQKo149YfE2weMGXW/DYGRXl8RMUwGsur10nesfUBZfLPYW014rDm+\n" \
  "QjGa2bcYJMUnAtUz1ctaQNV8T4HM3SwXABSbuczDGM4FpFCd51tjJDh8vxdmZpGJ\n" \
  "KEhWsvXcrlzBpVyW5CX/TixVYzautBdOM2cN+yniLjHAkHBWCF39LoAQatbHNFSq\n" \
  "FpADIpMiGFyGMxf029s2JgdNvkgR2aUL0ed2hGP9kKyLio+RNF5HD7mbbBM4d06P\n" \
  "t79aRgHvQAOeHJPfz9LleOoRUpg1gb8jmLDtKkWe+JGtsEDCPeb0HTvlL4ttGrZ4\n" \
  "LoIPCVbz\n" \
  "-----END ENCRYPTED PRIVATE KEY-----\n"

#define X_9930 \
  "-----BEGIN ENCRYPTED PRIVATE KEY-----\n" \
  "MIICojAcBgoqhkiG9w0BDAEDMA4ECM70GUHLNxJ7AgIIAASCAoBSzIR/pzL/Kz0k\n" \
  "QYJburqvHquGAa/xevMdelJdqAKPfqMuaOOhbZUkpp1Yf/jswyrzImgOnkb2stO8\n" \
  "hsa3gTZLk3j1LA5JXb89Pm+dqv1gXWJco7dnq8JJEhTt7Mr6rm/P1uV9UBXlgv+E\n" \
  "2F+b8GBDikMw38zqRGtg3GPjFaZKcL7tqwRm390t57cWSbqLLaNmRIxcf5TARHEs\n" \
  "TZEU+BHF2JoFE7rXPdUJAJwsw35C5JS4DXwEUBVoEeI3jXl3yDOqu20uekbrndL5\n" \
  "seACup8mQp5nHUBNk6RMg7/8/hqeRU9IFyCstvFqjtvbPvJLEML8jSyd+XoZU1tm\n" \
  "VpnU7KcN3bSN/BK4QzChGr5sD/2rteceBIJjDsHR7FjHJQIKlTxMok3taM84knnw\n" \
  "QcO0T0vbsmUqbs1MltGcUgm3p6Jp/NyeHZGfDqu4TEZcHE+mrNVVReRHL3O55UpC\n" \
  "AyZeJDu9nQKe62Y6oGcOUOuZkoodfh9M1V44f9guOv5b+2VIFgUIZTOVHLkmb3Nx\n" \
  "r6rUn2++N02II7zkvR1aHILZw/JnqHQC5bpK6qlTNUN3kNy5DHg4iAHGuKUxksK0\n" \
  "qziPL/VYfos0/81O4mNI3yo3D2WA6usgy+MZyDY0u4uAbcz4irE1ACHj3cgBHx2j\n" \
  "RemyLdgPX+kPXr5wKHKk4U93nIgZXbshuuG5CrwtJqXslx6dG6FYChaUJsc/kCga\n" \
  "JFkHnOZk3tMxxyVBaBKUnyFxbxFBORgYGGAEKJ4RYT0ge8sSkVo4NNsNjLw74+d6\n" \
  "zlt7NLEhDn+IuaocYejf4Do5W+jIfkpZXF/w6DRHyJ3l2CHV/c9AN/lltTQYIg4Y\n" \
  "twhxefdG\n" \
  "-----END ENCRYPTED PRIVATE KEY-----\n"

#define X_9931 \
  "-----BEGIN ENCRYPTED PRIVATE KEY-----\n" \
  "MIICojAcBgoqhkiG9w0BDAEDMA4ECO6DyRswVDToAgIIAASCAoB3xqmr0evfZnxk\n" \
  "Gq/DsbmwGVpO1BQnv+50u8+roflrmHp+TdX/gkPdXDQCqqpK/2J/oaGMCtKEiO8R\n" \
  "/pxSKcCX3+7leF01FF4z3rEcTVRej0mR6IAzk5QZR4Y0jXzay7Quj2zFJQTASdRy\n" \
  "6o9HQt5YuDyMFY30yjungmg6sYLBLZ2XypCJYH3eUQx9BjwsbGqVnXRQ6oezL5tD\n" \
  "K+tRH41OK2pzFqhnpRvbfPtNDmUnMLUnahGBubRzNQgHE0iNGIYpOawpVabj15H2\n" \
  "4lQ9KBREaqLqiV/VMPFYcRd8tBjE2pRs3yhJ9bjl73gdh6qVvcXIqBBQcRtNbpQ/\n" \
  "WKFzVz5dMCEzS+LhMT2m0GtTYqn8IqRuDgF7P8+347k4wKvrA2XgwP0bvh+IBb4e\n" \
  "nMQuJaKsnMZZPgAPqfIqWsn3cw27iEb5ros+My4KMlMbKBvH2HTXx5YkYJfbRLJ1\n" \
  "oe0mUxshTSOJeOjsfkStsP7QCSIvVb76t2Jo6HKIXEylXFAzj39lea6aysx6KX4c\n" \
  "aC/9XDlhqs0GGcJE3ILbiePTWWiASWjS08ggQasMZsT4VYUaIl3ti1N1cK9xwkaD\n" \
  "BE12JvWEtPd7MtGouPGijXycAtNgPw17vWg/3O11vTKDAHse90dOOpqYpXFN9Cfi\n" \
  "wa72WOkxFEZDuzV/dmjXX1WN82MoXs7pkHLvTgCmdydQ0ZJABYZj1+ZnF5eR6zLo\n" \
  "LAJnV3gOY0DGLORuoifEWMRlzDyYQOBN9smK9xKDtA6CHUuB9jRHKBevQrFy4+Ed\n" \
  "trCmsp9qXPzGvmJOA1YEgnZZPvXjAB7TCv2VrftKgebzbQE2mOoF1YcT1PIB7dFL\n" \
  "AopQ9gdD\n" \
  "-----END ENCRYPTED PRIVATE KEY-----\n"

#define X_9932 \
  "-----BEGIN ENCRYPTED PRIVATE KEY-----\n" \
  "MIICojAcBgoqhkiG9w0BDAEDMA4ECEKkETmhIXPkAgIIAASCAoBzNPQiMSQC6RSk\n" \
  "5Lk5cAbP1r//rE3IA0MNVy2ZwM4UZAQYHCxHkMpParGXwKt3/me064RXRwKOg9UT\n" \
  "nGx5/2A/AI2061A5M0KPVFE41IWQWoVGaiCaAzUDSF2Y+SL9yuLVqEES0gDQgUv5\n" \
  "uVnGyrbSo7sT8MSdvBuzdgmVluiaEVQhfwWJ9f8Q+ebQ1WVkeftzCe9yp1PLj8Yl\n" \
  "VCQ6X5qXqsApJ34Y62wXGqNbEvBkRyKbSqfqMI837tAVdMCdbsEE7wavzxGW6F9h\n" \
  "+igbPZO1NSzY0FZX1eQYqKZxfbkQmyDPLFT2S7BVv2wmihnC/SeZTcOoM+QoWG9j\n" \
  "XNLr1oqbeNxOnELmOXSrOekzbI7GhUcphYEIOBG/4B7ZP3cZ6TEw1EygXUan09XZ\n" \
  "Uz/CFbBTfX1uXHkMSzWwowXpx12vjH78KrRn69WBMGn/YjUheDLjwCDhJQK2CRDH\n" \
  "LbNBvZ7ezy1qHX90jrIdQnQzAoynu1OCfbd+84U2VifAszTcRvPMdiLlJh9MeyFY\n" \
  "8xDmmeNYGTVuDvAuzTlqbGablgQJu80VZ8CgQSW/0x7+oPozichza9tOd19aMDJ4\n" \
  "f8REy/9DAn1jRq/Cy/JFQoTpq3NtcWf9+NPHCwOMjaL63m6fIPXw6s9hnq8WMVIS\n" \
  "mtf5Jkvf402+8jhw1IqTVJasOMTRn62KsRt9a4JcWtorECA42wZGXjge3K9HYk4T\n" \
  "IVXq39VmeRP/9WveDwjkIThMl+0v5fl6Baaz/krXOIRfL6LV3RpkqPF4j/wneXgZ\n" \
  "7cMySs/FL96y6A+yJv281IQadYCqj7nPy92IYESQIcYjA8nd8hvsOxpnaMjXZjui\n" \
  "UWl07o3w\n" \
  "-----END ENCRYPTED PRIVATE KEY-----\n"

static struct {
	const char *name;
	const char *password;
	const char *pkcs12key;
	int expected_result;
} keys[] = {
	{
	"x_9607", "123456", X_9607, 0}, {
	"x_9671", "123456", X_9671, 0}, {
	"x_9925", "123456", X_9925, 0}, {
	"x_9926", "123456", X_9926, 0}, {
	"x_9927", "123456", X_9927, 0}, {
	"x_9928", "123456", X_9928, 0}, {
	"x_9929", "123456", X_9929, 0}, {
	"x_9930", "123456", X_9930, 0}, {
	"x_9931", "123456", X_9931, 0}, {
	"x_9932", "123456", X_9932, 0}
};

int main(void)
{
	gnutls_x509_privkey_t key;
	size_t i;
	int ret;

	global_init();

	for (i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
		gnutls_datum_t tmp;

		ret = gnutls_x509_privkey_init(&key);
		if (ret < 0)
			return 1;

		tmp.data = (unsigned char *) keys[i].pkcs12key;
		tmp.size = strlen((char *) tmp.data);

		ret = gnutls_x509_privkey_import_pkcs8(key, &tmp,
							GNUTLS_X509_FMT_PEM,
							keys[i].password,
							0);
		gnutls_x509_privkey_deinit(key);

		if (ret != keys[i].expected_result) {
			printf("fail[%d]: %d: %s\n", (int) i, ret,
				gnutls_strerror(ret));
			return 1;
		}

	}

	gnutls_global_deinit();

	return 0;
}
