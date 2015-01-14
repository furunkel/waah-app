# Waah App

Waah allows you to create simple apps in the spirit of [Processing](https://processing.org/) in Ruby (using MRuby).

## Example

A Waah app looks like this:

```ruby
class MyApp < Waah::App
  # called once before entering the main loop
  def setup
    # set a callback for text input  
    keyboard.text do |t|
      puts "user typed #{t}"
    end
  end
  
  def draw
    # draw a button at (10, 10)
    rounded_rect 10, 10, 100, 20
    
    # paint it red
    color 0xff, 0, 0
    fill true
    
    # add a blue border
    color 0, 0, 0xff
    stroke true
    
    # if clicked print a message
    # mouse.in? refers to the current
    # path (the rounded rectangle in our case)
    # and works for any path whatsoever.
    if mouse.down? && mouse.in?
      puts 'button clicked'
    end
  end
end
```
`Waah::App` inherits all the drawing methods from [`Waah::Canvas`](https://github.com/furunkel/waah-canvas).

## Supported Platforms

Currently the following platforms are supported:

* Linux (X11)
* Windows
* Android (Software renderer)

### Android Support

You can easily run your Waah sketches on Android (and cross compile Waah and potential native app code). Please see the [Makefile](/Makefile) and the [MRuby build file](/run_test.rb).

You will find an example project in [/android](/android). Copy the compiled shared library as `libwaah.so` to the appropriate `lib` folder in the Android project root. Then place your sketch in `assets/app.rb`. It is automatically loaded and run upon app startup.

#### Special Methods
You can load images and font files from the asset folder using the `Waah::Image#asset` and `Waah::Font#asset` methods.
Further you can use `Waah::App#log` to give output on logcat.

The keyboard can be brought up using `Waah::Keyboard#show`.
`Waah::App#android?` can be used to detect wether a sketch is being run on Android.


