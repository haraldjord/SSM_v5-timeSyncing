/*******************************************************************************
 * Copyright (c) 2014-2015 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Zurich Research Lab - initial API, implementation and documentation
 *******************************************************************************/

#ifndef DEBUG_H_
#define DEBUG_H_


char debug_str_buf[256];

// intialize debug library
void debug_init (void);

// set LED state
void debug_led (char val);

// write character to USART
void debug_char (char c);

// write nul-terminated string to USART
void debug_str (char* str);

// write LMiC event name to USART
void debug_event (int ev);

#endif // DEBUG_H_