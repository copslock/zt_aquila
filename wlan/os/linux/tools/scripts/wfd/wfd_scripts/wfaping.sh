#!/bin/sh

#/****************************************************************************
# *  (c) Copyright 2006 Wi-Fi Alliance.  All Rights Reserved 
# *
# * 
# *  LICENSE
# *
# * License is granted only to Wi-Fi Alliance members and designated Wi-Fi 
# * contractors ("Authorized Licensees").  Authorized Licensees are hereby 
# * granted the limited right to use this software solely for noncommercial 
# * applications and solely for testing Wi-Fi equipment. Authorized Licensees 
# * may embed this software into their proprietary equipment and distribute this
# * software with such equipment under a license with at least the same 
# * restrictions as contained in this License, including, without limitation, 
# * the disclaimer of warranty and limitation of liability, below.  Other than 
# * expressly granted herein, this License is not transferable or sublicensable, 
# * and it does not extend to and may not be used with non-Wi-Fi applications.
# *
# * Commercial derivative works of this software or applications that use the 
# * Wi-Fi scripts generated by this software are NOT AUTHORIZED without specific
# * prior written permission from Wi-Fi Alliance.
# *
# * Non-Commercial derivative works of this software for internal use are 
# * authorized and are limited by the same restrictions; provided, however, 
# * that the Authorized Licensee shall provide Wi-Fi with a copy of such 
# * derivative works under a perpetual, payment-free license to use, modify, 
# * and distribute such derivative works for purposes of testing Wi-Fi equipment.
# *
# * Neither the name of the author nor "Wi-Fi Alliance" may be used to endorse 
# * or promote products that are derived from or that use this software without 
# * specific prior written permission from Wi-Fi Alliance.
# * 
# * THIS SOFTWARE IS PROVIDED BY WI-FI ALLIANCE "AS IS" AND ANY EXPRESSED OR 
# * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
# * OF MERCHANTABILITY, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE, 
# * ARE DISCLAIMED. IN NO EVENT SHALL WI-FI ALLIANCE BE LIABLE FOR ANY DIRECT, 
# * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
# * (INCLUDING, BUT NOT LIMITED TO, THE COST OF PROCUREMENT OF SUBSTITUTE GOODS 
# * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
# * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
# * LIABILITY, OR TORT (INCLUDING NEGLIGENCE) ARISING IN ANY WAY OUT OF THE USE 
# * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# ******************************************************************************
# */
#
#echo $@
if [ -e /usr/bin/ping ]; then
    if echo "$@" | grep -q -- "-i 0"; then
        /usr/bin/ping $@ &
    else
        /usr/bin/ping $@ &
    fi
else
    if echo "$@" | grep -q -- "-i 0"; then
        /bin/ping $@ &
    else
        /bin/ping $@ &
    fi
fi
echo PID=$! > /tmp/pingpid.txt 
