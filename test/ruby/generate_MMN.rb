require 'gosu'
require 'csv'

class GameWindow < Gosu::Window
  def initialize
    super 600, 600
    self.caption = "SSVEP generator"
    @frame = 0
    @stimfreq = 30 # in number of frame
    @stimlenght = 4

    @prob = 85
    @stimcount = 0
    @timestamp = []
    puts "Stimulus frequency : #{60/@stimfreq} Hz"

  end

  def update

  end

  def draw
      if ((@frame) % (@stimfreq))<@stimlenght
          if ((@frame) % (@stimfreq))==0
              @stimcount += 1
              if 100*rand > @prob
                  @color = 0xffffffff
                  @odd = 1
              else
                  @color = 0xffffffff
                  @odd = 0
              end
              @timestamp << [(Time.now().to_f * 1000).floor, @odd]
          end
      else
          @color = 0x00000000
      end
      @frame += 1

      x = 300
      y = 300
      size = 200
      draw_quad(x-size, y-size, @color, x+size, y-size, @color, x-size,
                y+size, @color, x+size, y+size, @color, 0)
  end

  def button_down(id)
      if id == Gosu::KbEscape
          CSV.open("MMN_timestamp.csv", "wb") do |csv|

              csv << ["timestamp", "target"]
              @timestamp.each do |t|
                  csv << t
              end
          end
          close
      end
  end
end

window = GameWindow.new
window.show
