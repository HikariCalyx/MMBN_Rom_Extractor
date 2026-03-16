use anyhow::{Context, Result};
use std::fs::{self, File};
use std::io;
use std::path::{Path, PathBuf};

use steamlocate::SteamDir;
use zip::ZipArchive;
use sevenz_rust2::ArchiveWriter;
use indicatif::{ProgressBar, ProgressStyle};
use console::Term;

fn press_any_key() {
    println!("Press any key to continue...");
    let term = Term::stdout();
    let _ = term.read_char();
}

fn main() -> Result<()> {
    const APP_IDS: [(u32, &str); 2] = [
        (1798010, "Vol_1"),
        (1798020, "Vol_2"),
    ];
    const OUTPUT_7Z: &str = "roms.7z";

    let extract_temp_dir = PathBuf::from("extracted_srl_temp");

    println!("🔍 Locating Steam and both Mega Man Battle Network Legacy Collections...");

    let steam = SteamDir::locate()
        .context("Failed to locate Steam. Is Steam installed?")?;

    let mut extracted_any = false;

    if extract_temp_dir.exists() {
        fs::remove_dir_all(&extract_temp_dir)?;
    }
    fs::create_dir_all(&extract_temp_dir)?;
    for (appid, vol_name) in APP_IDS {
        let Some((app, library)) = steam.find_app(appid)? else {
            println!("⚠️  AppID {appid} ({vol_name}) not installed - skipping");
            continue;
        };

        let game_dir = library.path()
            .join("steamapps")
            .join("common")
            .join(&app.install_dir);

        println!("✅ Found {} at {}", app.name.as_deref().unwrap_or("Unknown"), game_dir.display());

        let data_dir = game_dir.join("exe").join("data");
        if !data_dir.exists() {
            println!("   No exe/data/ folder - skipping");
            continue;
        }

        let dat_files: Vec<PathBuf> = fs::read_dir(&data_dir)?
            .filter_map(|entry| {
                let path = entry.ok()?.path();
                if path.is_file() && path.extension().map_or(false, |e| e.to_string_lossy().eq_ignore_ascii_case("dat")) {
                    Some(path)
                } else {
                    None
                }
            })
            .collect();

        if dat_files.is_empty() {
            println!("   No .dat (ZIP) files found");
            continue;
        }

        println!("   Found {} .dat archives → extracting .srl files...", dat_files.len());

        for dat_path in dat_files {
            let dat_stem = dat_path.file_stem()
                .and_then(|s| s.to_str())
                .unwrap_or("unknown")
                .to_string();

            let zip_file = File::open(&dat_path)?;
            let mut archive = ZipArchive::new(zip_file)?;

            let base_out = extract_temp_dir.join(vol_name).join(&dat_stem);
            fs::create_dir_all(&base_out)?;

            let mut extracted_this = false;
            for i in 0..archive.len() {
                let mut zipped = archive.by_index(i)?;
                let zip_entry_name = zipped.name().to_string();

                if !zip_entry_name.to_lowercase().ends_with(".srl") {
                    continue;
                }

                let target_rel_path = match Path::new(&zip_entry_name).strip_prefix(Path::new(&dat_stem)) {
                    Ok(stripped) => stripped.to_path_buf(),
                    Err(_) => PathBuf::from(&zip_entry_name),
                };

                let out_path = base_out.join(&target_rel_path);

                if let Some(parent) = out_path.parent() {
                    fs::create_dir_all(parent)?;
                }

                let mut out_file = File::create(&out_path)?;
                io::copy(&mut zipped, &mut out_file)?;

                println!("      Extracted: {} → {}/{}/{}", 
                    zip_entry_name, 
                    vol_name, 
                    dat_stem, 
                    target_rel_path.display()
                );

                extracted_this = true;
                extracted_any = true;
            }

            if !extracted_this {
                let _ = fs::remove_dir(&base_out);
            }
        }
    }

    if !extracted_any {
        anyhow::bail!("No .srl files were found.");
    }
    
    println!("🔄 Renaming .srl → .gba and moving to flat root directory...");

    let rules: Vec<(&str, &str, &str)> = vec![
        ("exe1",  "rom.srl",     "exe1.gba"),
        ("exe1",  "rom_e.srl",   "bn1.gba"),
        ("exe2j", "rom.srl",     "exe2.gba"),
        ("exe2j", "rom_e.srl",   "bn2.gba"),
        ("exe3",  "rom.srl",     "exe3.gba"),
        ("exe3",  "rom_e.srl",   "bn3w.gba"),
        ("exe3b", "rom_b.srl",   "exe3b.gba"),
        ("exe3b", "rom_b_e.srl", "bn3b.gba"),
        ("exe4",  "rom.srl",     "exe4rs.gba"),
        ("exe4",  "rom_e.srl",   "bn4rs.gba"),
        ("exe4b", "rom_b.srl",   "exe4bm.gba"),
        ("exe4b", "rom_b_e.srl", "bn4bm.gba"),
        ("exe5",  "rom.srl",     "exe5tb.gba"),
        ("exe5",  "rom_e.srl",   "bn5tp.gba"),
        ("exe5k", "rom_k.srl",   "exe5tc.gba"),
        ("exe5k", "rom_k_e.srl", "bn5tc.gba"),
        ("exe6",  "rom.srl",     "exe6g.gba"),
        ("exe6",  "rom_e.srl",   "bn6g.gba"),
        ("exe6f", "rom_f.srl",   "exe6f.gba"),
        ("exe6f", "rom_f_e.srl", "bn6f.gba"),
    ];

    for vol_name in ["Vol_1", "Vol_2"] {
        for &(folder, src_file, dst_file) in &rules {
            let source = extract_temp_dir.join(vol_name).join(folder).join(src_file);
            if source.exists() {
                let target = extract_temp_dir.join(dst_file);

                if target.exists() {
                    let _ = fs::remove_file(&target);
                }

                fs::rename(&source, &target)?;
                println!("   ✅ Renamed & moved: {}/{} → {}", folder, src_file, dst_file);
            }
        }
    }
    
    println!("🧹 Removing leftover .srl files and empty folders...");

    for vol_name in ["Vol_1", "Vol_2"] {
        let vol_dir = extract_temp_dir.join(vol_name);
        if vol_dir.exists() {
            let _ = fs::remove_dir_all(&vol_dir);
        }
    }

    println!("🗜️ Packing all renamed .gba files into flat 7z...");

    let pb = ProgressBar::new_spinner();
    pb.set_style(
        ProgressStyle::default_spinner()
            .tick_chars("⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏ ")
            .template("{spinner:.green} {msg}")
            .unwrap(),
    );
    pb.enable_steady_tick(std::time::Duration::from_millis(100));
    pb.set_message("Creating roms.7z ...");

    let mut writer = ArchiveWriter::create(OUTPUT_7Z)
        .context("Failed to create 7z writer")?;
    let lzma2_method = sevenz_rust2::encoder_options::Lzma2Options::from_level(9).into();
    writer.set_content_methods(vec![lzma2_method]);

    writer.push_source_path(&extract_temp_dir, | _ | true)
        .context("Failed to push source path (solid packing)")?;

    writer.finish()
        .context("Failed to finish 7z archive")?;

    pb.finish_with_message("🎉 7z created!");

    println!("   Final archive → {}", OUTPUT_7Z);
    
    press_any_key();
    Ok(())
}