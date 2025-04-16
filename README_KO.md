# BetterSpray 

**BetterSpray**는 MetaHookSV용 플러그인으로, Sven Co-op의 스프레이 시스템을 향상시키며 여러 이미지, 실제 비율 유지, 동적 리로드를 지원합니다.

![BetterSpray 미리보기](https://raw.githubusercontent.com/KazamiiSC/BetterSpray---Sven-Co-op/refs/heads/main/preview/20250416020128_1.jpg)

## 🌟 주요 기능

### 🖼️ 스프레이 관리
- ✅ 여러 스프레이 지원 (PNG/JPG)
- ✅ `custom_sprays` 폴더의 이미지 자동 감지
- ✅ 게임 재시작 없이 동적 리로드 (`sprayreload`)
- ✅ 정리된 스프레이 목록 표시 (`spraylist`)

### 🎨 고급 커스터마이징
- 🔧 크기 조절 (`sprayscale 5-60`)
- 🔄 자유 회전 (`sprayrotate 0-360`)
- 💡 밝기 조절 (`spraybrightness 0.1-2.0`)
- ⏳ 지속 시간: 60초 + 2초 페이드 아웃
- 📐 **원본 이미지 비율 유지** (예: 720x420 등)

### 🚀 성능 및 품질
- 🖌️ OpenGL 렌더링
- 🔍 최대 1024x1024 텍스처 지원
- 💾 메모리 최적화

## 📥 설치 방법

Releases 탭에서 다운로드하고 Sven Co-op 메인 폴더에 넣어주세요.  
또는 다음 단계들을 따라 수동 설치하세요:

1. `BetterSpray.dll` 파일을 복사:  
   `Sven Co-op/svencoop/metahook/plugins/`

2. `SDL2.dll`, `SDL3.dll`, `svencoop.exe` 파일을 복사 및 교체:  
   `Steam/steamapps/common/Sven Co-op`  
   *(기존 `svencoop.exe`는 백업해두세요)*

3. 다음 폴더 생성:  
   `Sven Co-op/custom_sprays/`

4. 이미지 추가 (PNG/JPG):  
   - `spray1.png`  
   - `my_logo.jpg`  
   - `any_name.jpeg`

## 🕹️ 사용 방법

| 명령어               | 설명                                  | 예시                    |
|----------------------|---------------------------------------|-------------------------|
| `spray`              | 현재 선택된 스프레이 표시             | -                       |
| `spraynext`          | 다음 스프레이로 전환                 | -                       |
| `spraynext 이름`     | 특정 스프레이로 전환                 | `spraynext logo.png`    |
| `spraylist`          | 사용 가능한 모든 스프레이 목록 출력   | -                       |
| `sprayreload`        | 스프레이 폴더 새로 고침              | -                       |

## ⚙️ 시각 설정

| 명령어               | 범위         | 설명                           | 예시                     |
|----------------------|--------------|--------------------------------|--------------------------|
| `sprayscale`         | 5.0 - 60.0   | 스프레이의 세로 크기 조절      | `sprayscale 50`         |
| `sprayrotate`        | 0 - 360      | 회전 각도 (도 단위)             | `sprayrotate 45`        |
| `spraybrightness`    | 0.1 - 2.0    | 밝기/강도 조절                 | `spraybrightness 1.5`   |

## 📌 중요 사항

- ✨ **신규**: 이제 이미지가 원본 비율로 표시됩니다!
- 📏 최대 허용 크기: 1024x1024 픽셀
- 🔄 새 이미지를 추가한 후 `sprayreload` 명령어를 사용하세요
- 🎨 지원 포맷: PNG, JPG, JPEG

### 🎯 추천 설정

```plaintext
sprayscale 35
spraybrightness 1.2
🚀 로드맵
즐겨찾기 시스템

멀티플레이어 지원

특수 효과 (글로우, 테두리 등)

❓ 자주 묻는 질문 (FAQ)
Q: 내 이미지가 늘어나 보이는 이유는?
A: 이제는 그렇지 않습니다! 플러그인이 원본 비율을 유지합니다.

Q: 스프레이 목록을 어떻게 갱신하나요?
A: 새 이미지를 추가한 후 sprayreload 명령어를 입력하세요.

Q: 이미지 최대 크기는 얼마인가요?
A: 최대 크기는 1024x1024 픽셀입니다.

🛠 현재 버전: 1.4
❤️ 제작자: KZ-Sheez
