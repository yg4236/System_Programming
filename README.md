# Raspberry pi_project

## 프로젝트 개요
1. **동기** <br>
인간의 3대 욕구로 식욕, 성욕 그리고 수면욕이 있다. 여기서 수면욕은 생존을 위해 필수불가결한 욕
구이며, 3대 욕구 중 가장 강하다고 한다. 이렇게 강한 수면욕을 이기지 못하고 일상을 놓쳐버리는
사람들이 많다. 잠을 자느라 지각을 하고, 약속을 지키지 못하며, 서두르다가 중요한 준비물을 놓치
기도 한다. 그럼에도 우리가 중, 고등학교 시절을 잘 보낼 수 있었던 것은 부모님의 잔소리와 등짝 스
매싱 덕분이었다. 하지만 성인이 되어 독립하게 된 사람들은 위와 같은 실수를 반복하고 부모님의 빈
자리를 크게 느낄 것이다. 이 프로젝트는 혼자 생활하여 타인의 도움을 받기 힘든 사람들을 위해 시
작되었다.
2. **개발목표** <br>
고객군으로 설정한 1인가구에게 일어나는 문제를 3가지로 나누어 보았다. 첫째로 알람이 울려도 일
어나지 못해 일정에 늦는다. 둘째, 중요한 준비물을 챙기지 못한다. 마지막으로 외출할 때 집에 불을
켜 두고 나간다는 것이다. 이 프로젝트는 이 세가지 문제에 대한 해결법을 제시한다.
3. **기능** <br>
+ 기능1 : 시간 맞춰 기상 시키기 <br>
기능1은 사용자를 원하는 시간에 맞춰 기상시키는 것이다. 우선 알람 소리에 사용자가 바로 기상하
였는지 판단하여, 기상하지 않았다면 침실 등을 점등한다. 점등후에도 사용자가 기상하지 않았다면
물리적인 타격을 가해 사용자를 기상시킨다.
+ 기능2 : 중요 준비물 잘 챙겨 주기 <br>
기능2는 사용자가 급하게 준비하고 나가느라 놓칠 수 있는 준비물들을 잘 챙길 수 있게 도와주는 것
이다. 사용자가 현관문을 열고 나갈 때, 중요 준비물을 챙기지 않았다면 현관문 옆의 부저를 울려 사
용자에게 알린다.
+ 기능3 : 스마트 홈 <br>
기능3은 사용자가 급하게 외출하느라 소등하지 못하는 상황을 파악하고, 소등해주는 스마트 홈 기
능이다.
<br>
실행에는 3개의 파이 및 알맞는 회로 필요.<br>
1,3번 pi는 client, 2번 pi는 server의 역할을 수행한다.

### pi1
```
$gcc pi1.c -o pi1
$./pi1 서버IP주소 포트번호
```
### pi2
```
$gcc pi2_server.c -o pi2
$./pi2 pi1과통신할포트번호 pi3와통신할포트번호
```
### pi3
```
$gcc pi3_client.c -o pi3
$./pi3 서버IP주소 포트번호
```
### with 박상혁, 배찬희
> 2020.12.14 코드 최종 수정
