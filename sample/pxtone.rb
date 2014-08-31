bin = "#{__dir__}/../bin"
tunes = "#{__dir__}/tunes"
fonts = "#{__dir__}/fonts"

require_relative "#{bin}/Pxtone.so"
require 'dxruby'

Window.mag_filter = TEXF_POINT
Input.set_pad_repeat(P_LEFT, 15, 3)
Input.set_pad_repeat(P_RIGHT, 15, 3)
Input.set_pad_repeat(P_UP, 15, 3)
Input.set_pad_repeat(P_DOWN, 15, 3)

Font.install("#{fonts}/KonatuTohaba.ttf")
font = Font.new(11, "小夏 等幅")
line_height = font.size + font.size / 2
render_target = RenderTarget.new(Window.width / 2, Window.height / 2)
text = "press z key to play / pause"
vol_bar = Image.new(2, 8, [255, 0, 255, 0])

left = -> x = 1 { font.size * x - font.size / 2 }
bottom = -> y = 1 { render_target.height - (line_height * y + line_height / 2) }
center_x = -> x = 0 { (render_target.width - x) / 2 }
center_y = -> y = 0 { (render_target.height - y) / 2 }
draw_text = -> x, y, text {
  render_target.draw_font(x, y, text, font)
}
draw_text_list = -> *args {
  [*args].each_with_index {|text, i|
    draw_text[left[2 - i % 2], bottom[i + 1], text] 
  }
}
draw_scale = -> x, y, scale {
  Window.draw_ex(x, y, render_target, scalex: scale, scaley: scale)
}
draw_volume = -> volume {
  volume.times {|i|
    render_target.draw_ex(10 + i * 3, 10, vol_bar)
  }
}

# ピスコラの曲ファイルを読み込む
Pxtone.load_tune("#{tunes}/meique.pttune")

# 曲名を取得
title = Pxtone.tune_name
Window.caption = title
# 曲のコメントを取得
comment = Pxtone.tune_comment

volume = 100

Window.loop do
  if Input.pad_push?(P_BUTTON0)
    # 再生中かどうか
    if Pxtone.playing?
      # 再生中ならポーズする
      Pxtone.pause
    # ポーズ中
    elsif Pxtone.paused?
      # ポーズした位置から再開する
      Pxtone.resume
    else
      # 初回の再生; 0で頭から再生; ボリュームは 0.0～1.0の範囲の値にする
      Pxtone.play(0, volume / 100.0)
    end
  end
  if Input.pad_push?(P_LEFT) || Input.pad_push?(P_RIGHT)
    volume += Input.x
    volume = 0 if volume < 0
    volume = 100 if volume > 100
    # ボリュームの設定
    Pxtone.set_volume(volume / 100.0)
  end
  draw_volume[volume]
  draw_text[center_x[font.get_width(text)], center_y[font.size], text]
  draw_text_list[comment, "コメント", title, "曲名"]
  draw_scale[center_x[], center_y[], 2]
end
