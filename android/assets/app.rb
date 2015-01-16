class ExampleApp < Waah::App
  def setup
    rate 40.0
    log :verbose, "Running app" if android?

    @text = ""
    keyboard.text do |t|
      puts t
      #log :verbose, "text: #{t}"
      @text << t
    end
  end

  def draw
    clear

    @r ||= 0
    translate 100, 100 do
      rotate((@r % 100) / 100 * 2 * 3.14) do
        rounded_rect 0, 0, 50, 50, 3
      end
    end
    @r += 5

    if pointer.pressed? && pointer.in?
      color 0, 0xff, 0
      keyboard.toggle if android?
    else
      color 0xff, 0, 0
    end
    fill true
    color 0, 0xff, 0
    stroke

    font_size 20
    text 200, 100, @text
    color 0, 0, 0xff
    fill

    redraw
  end
end

app = ExampleApp.new 300, 300, "Example App"

begin
  app.run
rescue Exception => e
  app.log :error, e.inspect if android?
  puts e.inspect
end
