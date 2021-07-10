PC용 성능 & 장애 관리 시스템

사용자 PC에서 발생된 정보들을 커널 기반에서 수집하고 분석.
사용자를 불편하게 만들지만 몰랐던 소프트웨어 성능상 문제점, 장애 원인을 알려주고 해결해주는 공개 소프트웨어.
개발자도 관리자도 몰랐던 PC의 문제사항들을 밝혀 기존 상업용 소프트웨어의 민낯을 드러내는 재앙적 소프트웨어.
어려운 기술로 만들지만 쉽게 정보를 제공하는 컨셉.

2021년 가을 개인용 버전 출시 예정!



에이전트 구성 요소(개인용 + 기업용 공용)

* kernel driver
  File/Network/Registry/Process/Thread 데이터 수집 및 1차 통계 처리
* service
  driver 와 port 통신 - 데이터 수신 및 명령 송신
  2차 통계 처리(메모리 기반) 
  최종 결과 file db(sqlite3) 저장
  web host 와 ipc(named pipe) 통신 - 저장된 db 통계 정보, 메모리내 통계 처리되고 있는 실시간 정보 전송
  json -> sql -> json
* edge webview host container
  web ui 적재를 위한 edge chromium 기반 구조 (경량 크로미움 웹브라우저)
* vue web ui/html,java script
  사용자에게 보여주는 최종 ui - web/vue를 사용하여 기업용 관리자 화면과도 호환되도록 구현 가능.

서버 구성 요소(기업용)

* mqtt 
  server -> agent 
    상시 TCP 연결 
    server 명령 하달 
    다른 agent 명령 전달
  agent -> server
    heartbeat
    서버 허용 기반 다른 agent 제어(P2P) command

* P2P : agent <-> agent
  TCP 1:1 통신
  UDP 1:N 통신
  정책 교환
  명령 공유
  파일 공유
  데이터 공유 
  
* kafka
  agent -> server 데이터 전송 채널
  
* http/REST
  정책 하달
  파일 송수신
