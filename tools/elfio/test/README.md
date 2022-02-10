
#  
so_dynsym_chg 实现手动修改elf文件的符号属性，可以把函数符号bind属性GLOBAL与LOCAL修改；  
可以解决动态库内部函数与其他动态库函数名冲突问题

## 编译
`make clean && make`  

## 使用方法
* make完会生成liba.a静态库，liba.so动态库，testlib测试用例，so_dynsym_chg符号属性修改工具  
* 目前只验证动态库的符号bind属性修改，静态库待验证  
* make so_test 可以测试bind修改工具执行情况    
* 修改前后可以通过readelf -s liba.so查看符号属性  

## Tips
* 核心用到github开源库ELFIO工具   
* ELFIO工具，默认只支持ELF文件的读操作，不支持符号属性修改操作   
* 所以elfio/elfio_symbols.hpp修改源码添加了set_symbol和generic_set_symbol方法支持，从而达到能修改bind属性目的  

## 修改添加内容
```
    //------------------------------------------------------------------------------
    bool set_symbol( const char* name,
                         Elf64_Addr    value,
                         Elf_Xword     size,
                         unsigned char bind,
                         unsigned char type,
                         unsigned char other,
                         Elf_Half      shndx )
    {
        bool nRet;

        if ( symbol_section->get_size() == 0 ) {
			nRet = false;
			return nRet;
        }

        if ( elf_file.get_class() == ELFCLASS32 ) {
            nRet = generic_set_symbol<Elf32_Sym>( name, value, size, ELF_ST_INFO( bind, type ),
                                                  other, shndx );
        }
        else {
            nRet = generic_set_symbol<Elf64_Sym>( name, value, size, ELF_ST_INFO( bind, type ),
                                                  other, shndx );
        }

        return nRet;
    }

    //------------------------------------------------------------------------------
    template <class T>
    bool generic_set_symbol( const char*   name,
                                 Elf64_Addr    value,
                                 Elf_Xword     size,
                                 unsigned char info,
                                 unsigned char other,
                                 Elf_Half      shndx )
    {
        bool ret = false;
		T* pSym;
		const endianess_convertor& convertor = elf_file.get_convertor();
		
		for ( Elf_Xword i = 0; !ret && i < get_symbols_num(); i++ ) 
		{
			if ( nullptr != symbol_section->get_data() ) {
				const T* pConstSym = reinterpret_cast<const T*>(
					symbol_section->get_data() +
					i * symbol_section->get_entry_size() );
				pSym = const_cast<T*>(pConstSym);

				section* string_section =
					elf_file.sections[get_string_table_index()];
				string_section_accessor str_reader( string_section );
				const char*             pStr =
					str_reader.get_string( convertor( pSym->st_name ) );
				if ( strcmp(pStr, name) == 0 ) {
					ret = true;
				}
				
			}
		}
		if (ret != true) {
			return ret;
		}
		pSym->st_info = convertor( info );

        return ret;
    }
    //------------------------------------------------------------------------------
```
