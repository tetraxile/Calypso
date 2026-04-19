use slint::language::ColorScheme;
use tas_script_formats::{
	Buttons,
	glam::{IVec2, Vec2},
};
use tiny_skia::{Color, Paint, PathBuilder, PixmapMut, Rect, Stroke, Transform};

pub struct InputDisplay {
	buttons: Buttons,
	left_stick: Vec2,
	right_stick: Vec2,
}

impl Default for InputDisplay {
	fn default() -> Self {
		Self {
			buttons: Buttons::from_bits(u64::MAX),
			left_stick: Vec2::new(1.0, 0.0),
			right_stick: Vec2::new(-1.0, 0.0),
		}
	}
}

impl InputDisplay {
	#[allow(unused)]
	pub fn update(&mut self, buttons: Buttons, left_stick: IVec2, right_stick: IVec2) {
		self.buttons = buttons;
		self.left_stick = todo!("calculation: {}", left_stick.as_vec2());
		self.right_stick = todo!("calculation: {}", right_stick.as_vec2());
	}

	pub fn paint(&self, mut image: PixmapMut, _scheme: ColorScheme) {
		let size = Vec2::new(image.width() as _, image.height() as _);
		const INNER_SIZE: Vec2 = Vec2::new(340.0, 140.0);

		let transform = Transform::from_translate(size.x / 2.0, size.y / 2.0);

		let outer_ratio = size.x / size.y;
		let inner_ratio = INNER_SIZE.x / INNER_SIZE.y;

		let inner_size = if outer_ratio > inner_ratio {
			Vec2::new(size.y * inner_ratio, size.y)
		} else {
			Vec2::new(size.x, size.x / inner_ratio)
		};

		let scale = inner_size / INNER_SIZE;
		let transform = transform.pre_scale(scale.x, scale.y);

		{
			// todo: change based on color schemes?
			const ON_COLOR: Color = greyscale(0.3);
			const OFF_COLOR: Color = greyscale(0.7);
			image.fill(Color::TRANSPARENT);
			let mut paint = Paint {
				shader: tiny_skia::Shader::SolidColor(OFF_COLOR),
				..Default::default()
			};

			// sticks
			paint.set_color(Color::BLACK);
			Self::stroke_circle(&mut image, &paint, transform, -120.0, 15.0, 30.0);
			paint.set_color(OFF_COLOR);
			let pos = Vec2::new(-120.0, 15.0) + 20.0 * self.left_stick;
			Self::fill_circle(&mut image, &paint, transform, pos.x, pos.y, 20.0);
			paint.set_color(Color::BLACK);
			Self::stroke_circle(&mut image, &paint, transform, 120.0, 15.0, 30.0);
			paint.set_color(OFF_COLOR);
			let pos = Vec2::new(120.0, 15.0) + 20.0 * self.right_stick;
			Self::fill_circle(&mut image, &paint, transform, pos.x, pos.y, 20.0);

			let mut paint_for_button = |button: bool| {
				paint.set_color(if button { ON_COLOR } else { OFF_COLOR });
				paint.clone()
			};
			// dpad
			let paint = paint_for_button(self.buttons.left());
			Self::fill_circle(&mut image, &paint, transform, -60.0, 15.0, 10.0);
			let paint = paint_for_button(self.buttons.down());
			Self::fill_circle(&mut image, &paint, transform, -40.0, 35.0, 10.0);
			let paint = paint_for_button(self.buttons.up());
			Self::fill_circle(&mut image, &paint, transform, -40.0, -5.0, 10.0);
			let paint = paint_for_button(self.buttons.right());
			Self::fill_circle(&mut image, &paint, transform, -20.0, 15.0, 10.0);
			// abxy
			let paint = paint_for_button(self.buttons.a());
			Self::fill_circle(&mut image, &paint, transform, 60.0, 15.0, 10.0);
			let paint = paint_for_button(self.buttons.b());
			Self::fill_circle(&mut image, &paint, transform, 40.0, 35.0, 10.0);
			let paint = paint_for_button(self.buttons.x());
			Self::fill_circle(&mut image, &paint, transform, 40.0, -5.0, 10.0);
			let paint = paint_for_button(self.buttons.y());
			Self::fill_circle(&mut image, &paint, transform, 20.0, 15.0, 10.0);
			// +/-
			let paint = paint_for_button(self.buttons.y());
			Self::fill_circle(&mut image, &paint, transform, -20.0, -35.0, 7.0);
			let paint = paint_for_button(self.buttons.y());
			Self::fill_circle(&mut image, &paint, transform, 20.0, -35.0, 7.0);

			// zl/zr
			let paint = paint_for_button(self.buttons.zl());
			Self::fill_pill(&mut image, &paint, transform, -160.0, -60.0, 60.0, 20.0);
			let paint = paint_for_button(self.buttons.zr());
			Self::fill_pill(&mut image, &paint, transform, 100.0, -60.0, 60.0, 20.0);
			// l/r
			let paint = paint_for_button(self.buttons.l());
			Self::fill_pill(&mut image, &paint, transform, -85.0, -60.0, 40.0, 20.0);
			let paint = paint_for_button(self.buttons.r());
			Self::fill_pill(&mut image, &paint, transform, 45.0, -60.0, 40.0, 20.0);
		}
	}

	fn stroke_circle(
		image: &mut PixmapMut,
		paint: &Paint,
		transform: Transform,
		x: f32,
		y: f32,
		radius: f32,
	) {
		let ellipse = {
			let mut builder = PathBuilder::new();
			builder.push_circle(x, y, radius);
			builder.finish().unwrap()
		};
		image.stroke_path(&ellipse, &paint, &Stroke::default(), transform, None)
	}

	fn fill_circle(
		image: &mut PixmapMut,
		paint: &Paint,
		transform: Transform,
		x: f32,
		y: f32,
		radius: f32,
	) {
		let ellipse = {
			let mut builder = PathBuilder::new();
			builder.push_circle(x, y, radius);
			builder.finish().unwrap()
		};
		image.fill_path(
			&ellipse,
			&paint,
			tiny_skia::FillRule::Winding,
			transform,
			None,
		)
	}

	fn fill_pill(
		image: &mut PixmapMut,
		paint: &Paint,
		transform: Transform,
		x: f32,
		y: f32,
		width: f32,
		height: f32,
	) {
		let pill = {
			let mut builder = PathBuilder::new();
			let radius = height / 2.0;
			builder.push_circle(x + radius, y + radius, radius);
			builder.push_circle(x + width - radius, y + radius, radius);
			builder
				.push_rect(Rect::from_xywh(x + radius, y, width - radius * 2.0, height).unwrap());
			builder.finish().unwrap()
		};
		image.fill_path(&pill, &paint, tiny_skia::FillRule::Winding, transform, None)
	}
}

const fn greyscale(lum: f32) -> Color {
	unsafe { Color::from_rgba_unchecked(lum, lum, lum, 1.0) }
}
