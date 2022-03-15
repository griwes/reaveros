/*
 * Copyright © 2022 Michał 'Griwes' Dominiak
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

.global _start
.weak rose_main

.section .text._start

.extern __init_array_start

_start:
    // setup a (0, 0) frame at top of stack
    mov rbp, 0
    push rbp
    push rbp
    mov rbp, rsp

    call rose_main

    ud2

.section .text.__default.rose_main

rose_main:
    ud2     // trap; this will eventually be implemented in a .cpp file

