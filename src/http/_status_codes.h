#define STATUS_CODES(XX)                                                 \
	XX(100, 12, "100 Continue"                       )                   \
	XX(101, 23, "101 Switching Protocols"            )                   \
	XX(102, 14, "102 Processing"                     ) /* obsolete */    \
	                                                                     \
	XX(200,  6, "200 OK"                             )                   \
	XX(201, 11, "201 Created"                        )                   \
	XX(202, 12, "202 Accepted"                       )                   \
	XX(203, 33, "203 Non-Authoritative Information"  )                   \
	XX(204, 14, "204 No Content"                     )                   \
	XX(205, 17, "205 Reset Content"                  )                   \
	XX(206, 19, "206 Partial Content"                )                   \
	XX(207, 16, "207 Multi-Status"                   ) /* RFC 4918 */    \
	                                                                     \
	XX(300, 20, "300 Multiple Choices"               )                   \
	XX(301, 21, "301 Moved Permanently"              )                   \
	XX(302, 21, "302 Moved Temporarily"              )                   \
	XX(303, 13, "303 See Other"                      )                   \
	XX(304, 16, "304 Not Modified"                   )                   \
	XX(305, 13, "305 Use Proxy"                      )                   \
	XX(306, 16, "306 Switch Proxy"                   ) /* obsolete */    \
	XX(307, 22, "307 Temporary Redirect"             )                   \
	                                                                     \
	XX(400, 15, "400 Bad Request"                    )                   \
	XX(401, 16, "401 Unauthorized"                   )                   \
	XX(402, 20, "402 Payment Required"               )                   \
	XX(403, 13, "403 Forbidden"                      )                   \
	XX(404, 13, "404 Not Found"                      )                   \
	XX(405, 22, "405 Method Not Allowed"             )                   \
	XX(406, 18, "406 Not Acceptable"                 )                   \
	XX(407, 33, "407 Proxy Authentication Required"  )                   \
	XX(408, 19, "408 Request Timeout"                )                   \
	XX(409, 12, "409 Conflict"                       )                   \
	XX(410,  8, "410 Gone"                           )                   \
	XX(411, 19, "411 Length Required"                )                   \
	XX(412, 23, "412 Precondition Failed"            )                   \
	XX(413, 28, "413 Request Entity Too Large"       )                   \
	XX(414, 25, "414 Request-URI Too Large"          )                   \
	XX(415, 26, "415 Unsupported Media Type"         )                   \
	XX(416, 35, "416 Requested Range Not Satisfiable")                   \
	XX(417, 22, "417 Expectation Failed"             )                   \
	XX(418, 16, "418 I'm a teapot"                   ) /* RFC 2324 */    \
	XX(419,  0, nullptr                              ) /* placeholder */ \
	XX(420,  0, nullptr                              ) /* placeholder */ \
	XX(421,  0, nullptr                              ) /* placeholder */ \
	XX(422, 24, "422 Unprocessable Entity"           ) /* RFC 4918 */    \
	XX(423, 10, "423 Locked"                         ) /* RFC 4918 */    \
	XX(424, 21, "424 Failed Dependency"              ) /* RFC 4918 */    \
	XX(425, 24, "425 Unordered Collection"           ) /* RFC 4918 */    \
	XX(426, 20, "426 Upgrade Required"               ) /* RFC 2817 */    \
	XX(427,  0, nullptr                              ) /* placeholder */ \
	XX(428, 25, "428 Precondition Required"          ) /* RFC 6585 */    \
	XX(429, 21, "429 Too Many Requests"              ) /* RFC 6585 */    \
	XX(430,  0, nullptr                              ) /* placeholder */ \
	XX(431, 35, "431 Request Header Fields Too Large") /* RFC 6585 */    \
	                                                                     \
	XX(500, 25, "500 Internal Server Error"          )                   \
	XX(501, 19, "501 Not Implemented"                )                   \
	XX(502, 15, "502 Bad Gateway"                    )                   \
	XX(503, 23, "503 Service Unavailable"            )                   \
	XX(504, 20, "504 Gateway Time-out"               )                   \
	XX(505, 30, "505 HTTP Version Not Supported"     )                   \
	XX(506, 27, "506 Variant Also Negotiates"        ) /* RFC 2295 */    \
	XX(507, 24, "507 Insufficient Storage"           ) /* RFC 4918 */    \
	XX(508, 17, "508 Loop Detected"                  ) /* RFC 5842 */    \
	XX(509, 28, "509 Bandwidth Limit Exceeded"       )                   \
	XX(510, 16, "510 Not Extended"                   ) /* RFC 2774 */    \
	XX(511, 35, "511 Network Authentication Required") /* RFC 6585 */    \
