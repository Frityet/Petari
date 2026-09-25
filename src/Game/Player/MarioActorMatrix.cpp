#include "resource/TextEncoding.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MatrixControl.hpp"

static MatrixMap morph_bitmap[] = {{CP932("ジャンプ系バインド"), 0x1111100F},
                                   {CP932("スピンアタック"), 0x11231041},
                                   {CP932("コイン引っ張り"), 0x11110001},
                                   {CP932("カメ持ち"), 0x1111000F},
                                   {CP932("水解除"), 0x00001110},
                                   {CP932("ダメージ解除"), 0x00001100},
                                   {CP932("ノーダメージ解除"), 0x00000100},
                                   {CP932("アッパーパンチ"), 0x11110101},
                                   {CP932("スピン回復エフェクト"), 0x10002000},
                                   {""}};

static MatrixSelectList select_list[] = {{8, CP932("ノーマル"), CP932("メタル"), CP932("ファイア"), CP932("アイス"), CP932("ハチ"), CP932("ホッパー"), CP932("テレサ"), CP932("フー")},
                                         {2, CP932("×NG"), CP932("OK○")},
                                         {6, CP932("×NG"), CP932("OK○"), "FIRE", "ICE", CP932("消える"), CP932("回転のみ")},
                                         {2, CP932("×NG"), CP932("OK○")},
                                         {2, CP932("×NG"), CP932("OK○")},
                                         {2, CP932("×NG"), CP932("OK○")},
                                         {2, CP932("×NG"), CP932("OK○")},
                                         {2, CP932("×NG"), CP932("OK○")},
                                         {3, CP932("×NG"), CP932("OK○"), CP932("頭突き")},
                                         {3, CP932("×NG"), CP932("エフェクト＋音"), CP932("音のみ")}};

static MatrixMap auto_bind_bitmap[] = {{CP932("ウォータープレッシャーの弾"), 0x10112222},
                                       {CP932("ポール"), 0x11111000},
                                       {CP932("グリーンスーパースピンドライバー"), 0x11111222},
                                       {CP932("スーパースピンドライバー"), 0x11111222},
                                       {CP932("スピンドライバ"), 0x11111222},
                                       {CP932("Gキャプチャー"), 0x11111222},
                                       {CP932("移動用砲台"), 0x11111222},
                                       {CP932("ウォーターロード"), 0x11112222},
                                       {CP932("土管"), 0x33333103},
                                       {CP932("噴水（大）"), 0x00002000},
                                       {CP932("オオアワ[共有]"), 0x11112222},
                                       {CP932("スイングロープ"), 0x11111000},
                                       {CP932("空中ブランコ"), 0x11111000},
                                       {""}};

static MatrixSelectList auto_bind_selecterlist[] = {{8, CP932("ノーマル"), CP932("メタル"), CP932("ファイア"), CP932("アイス"), CP932("ハチ"), CP932("ホッパー"), CP932("テレサ"), CP932("フー")},
                                                    {4, CP932("×スルー"), CP932("OK○"), CP932("変身解除"), CP932("移動中不可")}};

static MatrixMap auto_efx_bitmap[] = {
    {CP932("わたげ発生源"), 0x20000000},
    {CP932("杭スイッチ"), 0x10000000},
    {CP932("ダミー敵")},
    {CP932("ザコカメムシ"), 0x18000400},
    {CP932("子連れカメムシ"), 0x18000400},
    {CP932("細野用マップパーツ75"), 0x04000000},
    {CP932("幽霊船たいまつ"), 0x01000000},
    {CP932("燭台"), 0x01000000},
    {CP932("燭台(テレサマンション)"), 0x01000000},
    {CP932("燭台(アイスボルケーノ)")},
    {CP932("バブル"), 0x01000000},
    {CP932("カボクリ"), 0x11000000},
    {CP932("エイ"), 0x10800000},
    {CP932("ポール"), 0x025A0000},
    {CP932("ポール（鉄骨）"), 0x025A0000},
    {CP932("オニマス(ピボット)"), 0x00200000},
    {CP932("オニマス"), 0x00200000},
    {CP932("移動用砲台"), 0x00040000},
    {CP932("スーパースピンドライバー"), 0x00040000},
    {CP932("スピンドライバ"), 0x00040000},
    {CP932("伸び植物"), 0x00090000},
    {CP932("空中ブランコ"), 0x00080000},
    {CP932("つる花"), 0x00080000},
    {CP932("土管"), 0x0040A800},
    {CP932("タイマーピースブロック"), 0x00004000},
    {CP932("ゴム星"), 0x00001000},
    {CP932("電撃レール")},
    {CP932("電撃レール点")},
    {CP932("ビリキュー")},
    {CP932("宝箱")},
    {CP932("宝箱(空っぽ)")},
    {CP932("宝箱ゴールド(空っぽ)")},
    {CP932("宝箱(ライフＵＰキノコ)")},
    {CP932("ひび割れ宝箱")},
    {CP932("アイス床")},
    {CP932("草")},
    {CP932("花")},
    {CP932("青い花")},
    {CP932("トルネード小石")},
    {CP932("サークルシェル")},
    {CP932("サークルストロベリー")},
    {CP932("タマコロチュートリアル")},
    {CP932("コイン花")},
    {CP932("隠れアイテム")},
    {CP932("スターピーススター")},
    {CP932("レバースイッチ")},
    {CP932("クリスタルケージ[小]")},
    {CP932("クリスタルケージ[中]")},
    {CP932("クリスタルケージ[大]")},
    {CP932("ファイアバー")},
    {CP932("電撃ビリビリレール")},
    {CP932("移動電撃ビリビリレール")},
    {CP932("スターピース")},
    {CP932("アイテムバブルピース")},
    {CP932("スターピースディレクターピース")},
    {CP932("フォロースターピース")},
    {CP932("グループスターピース")},
    {CP932("スターピースマザーピース")},
    {CP932("トゲ植物")},
    {CP932("トゲ植物(空中)")},
    {CP932("陸ウニゾー")},
    {CP932("ビリビリボール")},
    {CP932("飾り足")},
    {CP932("モグ")},
    {CP932("蝶")},
    {CP932("タコヘイ墨")},
    {CP932("マグナムキラー"), 0x10000000},
    {CP932("雪だるま"), 0x10000000},
    {CP932("よわブロック"), 0x10000000},
    {CP932("ジャンプ台"), 0x10000000},
    {CP932("スピニングボックス"), 0x10000000},
    {CP932("杭スイッチビッグ"), 0x10000000},
    {CP932("ヒビ石"), 0x10000000},
    {CP932("メカクッパパーツローラーA"), 0x00004000},
    {""},
};

static MatrixSelectList auto_efx_selecterlist[] = {{19,
                                                    CP932("スピンヒット振動"),
                                                    CP932("スピーカー音"),
                                                    CP932("スピンキャッチ許可"),
                                                    CP932("SHDホーミング許可(地形)"),
                                                    CP932("地形移動無視"),
                                                    CP932("ハチ出立"),
                                                    CP932("乗換センサ"),
                                                    CP932("青い炎エフェクト"),
                                                    CP932("水柱エフェクト"),
                                                    CP932("ダメージ中RUSH"),
                                                    CP932("ナナメつぶし許可"),
                                                    CP932("カメを捨てない"),
                                                    CP932("ハチ回復"),
                                                    CP932("ハチメータ消去"),
                                                    CP932("テレサ時は無視"),
                                                    CP932("リバインド制限"),
                                                    CP932("HD中ラッシュ"),
                                                    CP932("食い込み除外"),
                                                    CP932("水陸移動特殊処理"),
                                                    CP932("着地エフェクト"),
                                                    CP932("影を消す"),
                                                    CP932("ダメージ中跳ねる")},
                                                   {2, CP932("□マリオ側で立てる"), CP932("[v]オブジェ側で立てる")}};

static MatrixValueTable talking_head_height[] = {
    {CP932("クッパJｒ"), 300.0f}, {CP932("ルイージ"), 200.0f}, {CP932("ピーチ姫"), 180.0f},       {CP932("ペンギンコーチ"), 500.0f},       {CP932("ペンギン仙人"), 500.0f},
    {CP932("チコ"), 100.0f},      {CP932("デブチコ"), 200.0f}, {CP932("キノピオ"), 100.0f},       {CP932("天文台用キノピオ"), 100.0f},     {CP932("ペンギン"), 160.0f},
    {CP932("ロゼッタ"), 240.0f},  {CP932("ウサギ"), 100.0f},   {CP932("いたずらウサギ"), 100.0f}, {CP932("シャッチー（会話用）"), 220.0f}, {""}};

void MarioActor::initActionMatrix() {
    _FBC = new MatrixControl(CP932("変身中のマリオ行動表"), morph_bitmap, select_list, -1);
    _FC0 = new MatrixControl(CP932("マリオ状態とオートバインド対応表"), auto_bind_bitmap, auto_bind_selecterlist, 1);
    _FC4 = new MatrixControl(CP932("メッセージ送信応答時のエフェクト"), auto_efx_bitmap, auto_efx_selecterlist, 1);

    _FC8 = new MatrixValueGetter(CP932("キャラの身長"), talking_head_height);
}

bool MarioActor::isActionOk(const char* pName) const {
    return _FBC->getValue(pName, mPlayerMode);
}

u8 MarioActor::selectAction(const char* pName) const {
    return _FBC->getValue(pName, mPlayerMode);
}

bool MarioActor::selectAutoBind(const char* pName, u8* value) const {
    return _FC0->getValueOrNone(pName, mPlayerMode, value);
}

bool MarioActor::selectCustomEffectSpinHitSound(const char* pName) const {
    return _FC4->getBitOrNone(pName, 0);
}

bool MarioActor::selectSpinCatchInRush(const char* pName) const {
    return _FC4->getBitOrNone(pName, 2);
}

bool MarioActor::selectHomingInSuperHipDrop(const char* pName) const {
    return !_FC4->isExist(pName) ? false : _FC4->getBit(pName, 3);
}

bool MarioActor::selectNotHomingSensor(const HitSensor* pSensor) const {
    return !_FC4->isExist(pSensor->mHost->mName) ? false : !_FC4->getBit(pSensor->mHost->mName, 3);
}

bool MarioActor::selectInvalidMovingCollision(const char* pName) const {
    return !_FC4->isExist(pName) ? false : _FC4->getBit(pName, 4);
}

bool MarioActor::selectQuickResetBeeWallGravity(const char* pName) const {
    return !_FC4->isExist(pName) ? false : _FC4->getBit(pName, 5);
}

bool MarioActor::selectJumpRushSensor(const char* pName) const {
    return !_FC4->isExist(pName) ? false : _FC4->getBit(pName, 6);
}

bool MarioActor::selectDamageFireColor(const char* pName) const {
    return _FC4->getBitOrNone(pName, 7);
}

bool MarioActor::selectWaterInOutEffect(const char* pName) const {
    return _FC4->getBitOrNone(pName, 8);
}

bool MarioActor::selectOnDamageRush(const char* pName) const {
    return _FC4->getBitOrNone(pName, 9);
}

bool MarioActor::selectTiltPress(const HitSensor* pSensor) const {
    return _FC4->getBitOrNone(pSensor->mHost->mName, 10);
}

bool MarioActor::selectHandyRush(const HitSensor* pSensor) const {
    return _FC4->getBitOrNone(pSensor->mHost->mName, 11);
}

bool MarioActor::selectRecoverFlyMeter(const HitSensor* pSensor) const {
    return _FC4->getBitOrNone(pSensor->mHost->mName, 12);
}

bool MarioActor::selectHideFlyMeter(const HitSensor* pSensor) const {
    return _FC4->getBitOrNone(pSensor->mHost->mName, 13);
}

bool MarioActor::selectTeresaThru(const HitSensor* pSensor) const {
    return _FC4->getBitOrNone(pSensor->mHost->mName, 14);
}

bool MarioActor::selectRebindTimer(const HitSensor* pSensor) const {
    return _FC4->getBitOrNone(pSensor->mHost->mName, 15);
}

bool MarioActor::selectHipDropRush(const HitSensor* pSensor) const {
    return _FC4->getBitOrNone(pSensor->mHost->mName, 16);
}

bool MarioActor::selectPushOff(const HitSensor* pSensor) const {
    return _FC4->getBitOrNone(pSensor->mHost->mName, 17);
}

bool MarioActor::selectWaterInOutRush(const HitSensor* pSensor) const {
    return _FC4->getBitOrNone(pSensor->mHost->mName, 18);
}

bool MarioActor::selectLandEffect(const HitSensor* pSensor) const {
    return _FC4->getBitOrNone(pSensor->mHost->mName, 19);
}

bool MarioActor::selectNoShadow(const HitSensor* pSensor) const {
    return _FC4->getBitOrNone(pSensor->mHost->mName, 20);
}

bool MarioActor::selectDamagePop(const HitSensor* pSensor) const {
    return _FC4->getBitOrNone(pSensor->mHost->mName, 21);
}

f32 MarioActor::getFaceLookHeight(const char* pName) const {
    f32 value;
    return _FC8->getValue(pName, &value) ? value : 150.0f;
}
